#ifndef INCLUDE_GYOU_GITHUB_FETCH_VER_HPP_
#define INCLUDE_GYOU_GITHUB_FETCH_VER_HPP_

#include <expected>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields_fwd.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/date_time.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/url.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "gyou/http_requests.hpp"
#include "gyou/rss_parse_into_tree.hpp"
#include "gyou/structs/change_related/commit_specific.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{
    [[nodiscard]] corral::Task<std::expected<
        std::variant<gyou::CommitSpecific, std::string>, std::string>>
    github_fetch_version(auto& ioc, auto& semaphores,
                         gyou::EbuildSpecificData const& ebuild_data)
    {
        std::string new_version{};
        std::string new_commit{};
        uint64_t new_date = 0;

        boost::urls::url src_uri(ebuild_data.first_uri);

        std::string workspace;
        std::string repo;

        bool is_tag = false;
        for (int segment_pos_num = 0;
             auto const& seg : src_uri.encoded_segments())
            {
                switch (segment_pos_num)
                    {
                        case 0:
                            {
                                workspace = seg;
                                break;
                            }
                        case 1:
                            {
                                repo = seg;
                                break;
                            }
                        case 3:
                            {
                                if ("refs" == seg)
                                    {
                                        is_tag = true;
                                        // this is a legitimate use of goto.
                                        // stfu.
                                        // NOLINTNEXTLINE(hicpp-avoid-goto,cppcoreguidelines-avoid-goto)
                                        goto end_loop;
                                    }
                                break;
                            }
                        default:
                            break;
                    };
                ++segment_pos_num;
            }
    end_loop:
        std::string feed_url = "https://github.com/" + workspace + "/" + repo;
        if (is_tag)
            {
                feed_url += "/tags.atom";
            }
        else if (ebuild_data.commit_specific.has_value())
            {
                feed_url += "/commits.atom";
            }
        else
            {
                feed_url += "/releases.atom";
            }

        LOG_INFO("Presumable URL for feed: {}", feed_url);

        std::string request_str;
        {
            auto lock = co_await semaphores
                            .at(std::to_underlying(ebuild_data.service))
                            .lock();
            auto req_res
                = co_await request_internet(ioc, "", boost::urls::url(feed_url),
                                            boost::beast::http::verb::get,
                                            boost::beast::http::header<true>{});

            if (not req_res)
                {
                    co_return std::unexpected(req_res.error());
                }

            request_str = std::move(req_res.value());
        }
        LOG_TRACE_L3("response: {}", request_str);
        LOG_INFO("Received feed. {}", feed_url);
        boost::property_tree::ptree tree;
        try
            {
                tree = gyou::parse_rss_into_tree(request_str);
            }
        catch (boost::property_tree::xml_parser_error& e)
            {
                co_return std::unexpected(
                    fmt::format("Failed to parse feed: {}", e.what()));
            }
        std::string commit_version_or_tag;

        for (auto& xml_entry : tree.get_child("feed"))
            {
                if ("entry" != xml_entry.first)
                    {
                        continue;
                    }

                auto const& link_entry
                    = xml_entry.second.get_child("link.<xmlattr>.href");
                boost::urls::url parsed_link{link_entry.data()};
                LOG_DEBUG("Fetched url from: {}", parsed_link.c_str());
                for (int segment_num = 0;
                     auto const& seg : parsed_link.encoded_segments())
                    {
                        LOG_TRACE_L1("{} seg: {}", segment_num,
                                     std::string_view{seg});
                        if (3 == segment_num
                            && ebuild_data.commit_specific.has_value())
                            {
                                commit_version_or_tag = seg;
                            }
                        if (4 == segment_num)
                            {
                                commit_version_or_tag = seg;
                            }
                        ++segment_num;
                    }
                LOG_TRACE_L1(
                    "is_tag: {}, is it commit type: {} commit_version_or_tag: "
                    "{}",
                    is_tag, ebuild_data.commit_specific.has_value(),
                    commit_version_or_tag);
                if (is_tag or not ebuild_data.commit_specific.has_value())
                    {
                        new_version = std::move(commit_version_or_tag);
                    }
                else
                    {
                        new_commit = std::move(commit_version_or_tag);
                    }
                if (ebuild_data.commit_specific.has_value())
                    {
                        std::string date_str
                            = xml_entry.second.get_child("updated").data();

                        std::chrono::sys_time<std::chrono::seconds>
                            time_from_epoch{};

                        std::istringstream in_stream_for_parsing{date_str};

                        if (is_tag or ebuild_data.commit_specific.has_value())
                            {
                                in_stream_for_parsing >> std::chrono::parse(
                                    "%Y-%m-%dT%H:%M:%SZ", time_from_epoch);
                            }
                        else
                            {
                                in_stream_for_parsing >> std::chrono::parse(
                                    "%Y-%m-%dT%H:%M:%S%Ez", time_from_epoch);
                            }

                        LOG_TRACE_L1("Seconds: {}", time_from_epoch);
                        auto days = std::chrono::sys_days{
                            std::chrono::floor<std::chrono::days>(
                                time_from_epoch)};
                        LOG_TRACE_L1("Days: {}",
                                     days.time_since_epoch().count());
                        std::chrono::year_month_day ymd{days};
                        LOG_TRACE_L1("ymd: y: {} m: {} d: {}",
                                     static_cast<int>(ymd.year()),
                                     static_cast<unsigned>(ymd.month()),
                                     static_cast<unsigned>(ymd.day()));

                        // intention is to get a num that in decimal would look
                        // like 'yyyymmdd', Plain dumb math and fucking explicit
                        // casts to uint64_t.
                        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                        new_date = static_cast<uint64_t>(
                                       (static_cast<int>(ymd.year()) * 10000))
                                   + (static_cast<uint64_t>(
                                          static_cast<unsigned>(ymd.month()))
                                      * 100)
                                   + static_cast<uint64_t>(
                                       static_cast<unsigned>(ymd.day()));
                        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
                    }
                // do it only once to get info only for latest entry.
                break;
            }
        if (ebuild_data.commit_specific.has_value())
            {
                LOG_INFO("Fetched date: {} commit: {}", new_date, new_commit);
                co_return gyou::CommitSpecific{.date = new_date,
                                               .commit = new_commit};
            }
        LOG_INFO("Fetched version '{}'", new_version);
        if (new_version.starts_with("v"))
            {
                new_version = new_version.substr(1);
            }
        co_return new_version;
    }

}  // namespace gyou

#endif  // INCLUDE_GYOU_GITHUB_FETCH_VER_HPP_
