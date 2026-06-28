#define BOOST_STACKTRACE_USE_BACKTRACE 1
#define BOOST_ASIO_HAS_FILE 1
#define BOOST_ASIO_HAS_IO_URING 1
#include <array>
#include <chrono>
#include <expected>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

#include <CLI/CLI.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio.hpp>
#include <boost/asio/basic_stream_file.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/file_base.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/fields_fwd.hpp>
#include <boost/beast/http/message_fwd.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/url.hpp>
#include <corral/Nursery.h>
#include <corral/Semaphore.h>
#include <corral/asio.h>
#include <corral/corral.h>
#include <corral/detail/asio.h>
#include <corral/run.h>
#include <corral/wait.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/std.h>
#include <glaze/glaze.hpp>
#include <inja/environment.hpp>
#include <inja/inja.hpp>
#include <inja/json.hpp>
#include <magic_enum/magic_enum.hpp>
#include <quill/core/LogLevel.h>
#include <quill/std/Chrono.h>
#include <quill/std/FilesystemPath.h>
#include <quill/std/Vector.h>
#include <re2/re2.h>
#include <re2/set.h>
#include <reflex/pcre2matcher.h>

#include "gyou/array_utils.hpp"
#include "gyou/boost_stacktrace_format.hpp"
#include "gyou/http_requests.hpp"
#include "gyou/omega_exception.hpp"
#include "overwrite_log_macros.h"
#include "quill_static.h"
// it is for getting real type from compiler when debugging via
// `Debug<a_type_i_dont_know_and_i_want_understand> sth;`
template <typename T> struct Debug;

namespace
{

    template <class... Ts> struct overloads : Ts...
    {
        using Ts::operator()...;
    };

    constexpr size_t DEFAULT_MAX_CONCURRENT_REQUESTS_PER_SERVICE = 6;

    // NOLINTNEXTLINE(performance-enum-size)
    enum class Service : size_t
    {
        github = 0,
        gitlab = 1,
        bitbucket = 2,
        codeberg = 3,
        cpan = 4,
        cpan_module = 5,
        cpe = 6,
        cran = 7,
        ctan = 8,
        freedesktop_gitlab = 9,
        gentoo = 10,
        gnome_gitlab = 11,
        google_code = 12,
        hackage = 13,
        heptapod = 14,
        kde_invent = 15,
        launchpad = 16,
        osdn = 17,
        pear = 18,
        pecl = 19,
        pypi = 20,
        rubygems = 21,
        savannah = 22,
        savannah_nongnu = 23,
        sourceforge = 24,
        sourcehut = 25,
        vim = 26,
    };

    // Fucking C++ without C99 designated array initializer extension makes me
    // do this shit.
    constexpr auto ServicesNames = std::invoke(
        []()
            {
                constexpr auto not_a_map_because_of_fucking_cpp
                    = std::to_array({
                        // clang-format off
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::bitbucket),"https://bitbucket.org/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::codeberg),"https://codeberg.org/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::cpan),"https://metacpan.org/dist/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::cpan_module),"https://metacpan.org/pod/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::cpe),"cpe:/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::cran),"https://cran.r-project.org/web/packages/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::ctan),"https://ctan.org/pkg/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::freedesktop_gitlab),"https://gitlab.freedesktop.org/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::gentoo),"https://gitweb.gentoo.org/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::github),"https://github.com/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::gitlab),"https://gitlab.com/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::gnome_gitlab),"https://gitlab.gnome.org/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::google_code),"https://code.google.com/archive/p/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::hackage),"https://hackage.haskell.org/package/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::heptapod),"https://foss.heptapod.net/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::kde_invent),"https://invent.kde.org/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::launchpad),"https://launchpad.net/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::osdn),"https://osdn.net/projects/.*?/"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::pear),"https://pear.php.net/package/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::pecl),"https://pecl.php.net/package/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::pypi),"https://pypi.org/project/.*?/"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::rubygems),"https://rubygems.org/gems/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::savannah),"https://savannah.gnu.org/projects/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::savannah_nongnu),"https://savannah.nongnu.org/projects/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::sourceforge),"https://sourceforge.net/projects/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::sourcehut),"https://sr.ht/.*?"),
    std::make_pair<size_t,std::string_view>(std::to_underlying(Service::vim),"https://www.vim.org/scripts/script.php?script_id=.*?")
                        // clang-format on
                    });

                std::array<std::string_view,
                           std::size(not_a_map_because_of_fucking_cpp)>
                    arr;

                for (const auto& [pos, val] : not_a_map_because_of_fucking_cpp)
                    {
                        arr.at(pos) = val;
                    }

                return arr;
            });

    struct Config
    {
        boost::beast::http::fields headers;
        std::filesystem::path log_file;
        std::filesystem::path path_to_repo;
        std::filesystem::path path_to_portage_bin;
        std::filesystem::path path_to_portage_temp;
        std::filesystem::path path_to_portage_pym;
        std::filesystem::path path_to_gentoo_repo;
        size_t concurrency_per_service{};
        quill::LogLevel log_level{};
    };

    struct EntryData
    {
        std::string link_str;
        std::string author;
        std::string title;
        std::string description;
    };

    // NOLINTNEXTLINE(performance-enum-size)
    enum class ReturnCode : int
    {
        Success = 0,
        PartialSuccess = 1,
        AllHaveFailed = 2,
        NoEbuildFound = 3,
        FailDuringParsingCmdValues = 4,
        FailSpecifiedValueIsIncorrect = 5,
        FailDuringInitializationConfig = 6,
        FailStandardException = 7,
        FailParsePromptResult = 8,
        FailInitializationLogger = 9,
        ReceivedCancellationSignal = 10,

    };

    enum class PackageType
    {
        Unknown,
        ReleaseOrTag,
        Commit,
    };

    struct CommonContext
    {
        RE2 re_commit_str;
        RE2 re_src_uri;
        RE2 re_category;
        RE2 re_pkg_9999;
        RE2 re_pkg_with_date;
        RE2::Set re_set_services;
        reflex::PCRE2UTFMatcher re_version_matcher;
    };

    struct CommitSpecific
    {
        uint64_t date{};
        std::string commit;
    };

    struct EbuildSpecificData
    {
        std::filesystem::path filepath;
        std::string p;
        std::string pv;
        std::string pn;
        std::string category;
        std::string first_uri;
        std::optional<CommitSpecific> commit_specific;
        Service service{};
    };

    boost::property_tree::ptree parse_rss_into_tree(std::string const& rss_feed)
    {
        boost::property_tree::ptree tree;
        std::istringstream istr(rss_feed);
        boost::property_tree::read_xml(istr, tree);
        return tree;
    }

    [[nodiscard]] corral::Task<
        std::expected<std::string, boost::system::error_code>>
    file_to_string(auto& ioc, std::filesystem::path const& file_path)
    {
        boost::asio::stream_file file_reader(
            ioc, file_path, boost::asio::stream_file::read_only);

        std::string str_of_file;
        str_of_file.reserve(file_reader.size());

        auto&& [errc, bytes_read] = co_await boost::asio::async_read(
            file_reader, boost::asio::dynamic_buffer(str_of_file),
            corral::asio_nothrow_awaitable);
        if (errc && boost::asio::error::eof != errc)
            {
                co_return std::unexpected(errc);
            }
        co_return str_of_file;
    }

    [[nodiscard]] corral::Task<
        std::expected<std::filesystem::path, std::string>>
    bash_ebuild(auto& ioc, Config const& cfg,
                std::filesystem::path const& path_to_ebuild,
                std::string_view const pv)
    {
        std::string pkg_full_name = path_to_ebuild.filename().stem().string();
        std::string pkg_name = path_to_ebuild.parent_path().filename().string();
        std::string category
            = path_to_ebuild.parent_path().parent_path().filename().string();

        std::filesystem::path temp_folder
            = (cfg.path_to_portage_temp / category / pkg_full_name).concat("/");

        std::error_code errc_mkdir_p;
        std::filesystem::create_directories(temp_folder, errc_mkdir_p);
        if (errc_mkdir_p)
            {
                co_return std::unexpected(
                    fmt::format("Failed to create temp directory for a ebuild.",
                                errc_mkdir_p.message()));
            };

        std::unordered_map<boost::process::environment::key,
                           boost::process::environment::value>
            env_for_ebuild = {
                {"EBUILD", path_to_ebuild.string()},
                {"T", temp_folder.string()},
                {"PORTAGE_BIN_PATH", cfg.path_to_portage_bin.string()},
                {"PORTAGE_PYM_PATH", cfg.path_to_portage_pym.string()},
                {"PORTAGE_ECLASS_LOCATIONS_STR",
                 cfg.path_to_gentoo_repo.string() + ":"
                     + cfg.path_to_repo.string()},
                {"EBUILD_PHASE", "_internal_test"},
                {"CATEGORY", category},
                {"PF", pkg_full_name},
                {"PN", pkg_name},
                {"PV", pv},
            };

        boost::asio::readable_pipe rp_stdout{ioc};
        boost::asio::readable_pipe rp_stderr{ioc};

        auto proc = boost::process::process(
            ioc, (cfg.path_to_portage_bin / "ebuild.sh").string(),
            {"_internal_test", path_to_ebuild.string()},
            boost::process::process_stdio{.in = {/* in to default */},
                                          .out = rp_stdout,
                                          .err = rp_stderr},
            boost::process::process_environment{env_for_ebuild});

        LOG_DEBUG("Doing sth in bash, probably");

        std::string stdout_s;
        std::string stderr_s;

        auto [proc_tuple, _, _] = co_await corral::allOf(
            proc.async_wait(corral::asio_nothrow_awaitable),
            boost::asio::async_read(rp_stdout,
                                    boost::asio::dynamic_buffer(stdout_s),
                                    corral::asio_nothrow_awaitable),
            boost::asio::async_read(rp_stderr,
                                    boost::asio::dynamic_buffer(stderr_s),
                                    corral::asio_nothrow_awaitable));
        auto&& [_, errc_proc] = proc_tuple;

        if (errc_proc != 0)
            {
                co_return std::unexpected(fmt::format(
                    "Failed to do ebuild: ec: {}\nstderr: {}\nstdout: {}",
                    errc_proc, stderr_s, stdout_s));
            }

        co_return temp_folder;
    }

    // get current version of the pkg or commit of
    // current pkg extract service and link
    [[nodiscard]] corral::Task<std::expected<EbuildSpecificData, std::string>>
    get_ebuild_info(auto& ioc, Config const& cfg, CommonContext& common_ctx,
                    std::filesystem::directory_entry const& path_to_ebuild)
    {
        std::string category_str = path_to_ebuild.path()
                                       .parent_path()
                                       .parent_path()
                                       .filename()
                                       .string();

        auto pkg_type = PackageType::Unknown;

        std::string pkg_p = path_to_ebuild.path().filename().stem().string();
        uint64_t date = 0;

        if (RE2::FullMatch(pkg_p, common_ctx.re_pkg_with_date, &date))
            {
                pkg_type = PackageType::Commit;
                LOG_INFO(
                    "This is a package per "
                    "commit: "
                    "{} . Its date: {}",
                    pkg_p, date);
            }
        else
            {
                pkg_type = PackageType::ReleaseOrTag;
                LOG_INFO(
                    "This is a package per "
                    "release: {}",
                    pkg_p);
            }

        common_ctx.re_version_matcher.input(pkg_p);
        if (not common_ctx.re_version_matcher.find())
            {
                co_return std::unexpected(fmt::format(
                    "Failed to parse version of package : {}", pkg_p));
            }

        size_t match_id = 0;
        while (common_ctx.re_version_matcher[match_id].first != nullptr)
            {
                LOG_TRACE_L2(
                    "sth: {}",
                    std::string_view{
                        common_ctx.re_version_matcher[match_id].first,
                        common_ctx.re_version_matcher[match_id].second});
                if (common_ctx.re_version_matcher[match_id].first == nullptr)
                    {
                        LOG_DEBUG("It is nullptr.");
                    }
                ++match_id;
            }

        auto pkg_n = std::string{common_ctx.re_version_matcher[1].first,
                                 common_ctx.re_version_matcher[1].second};
        auto pkg_v = std::string{common_ctx.re_version_matcher[2].first,
                                 common_ctx.re_version_matcher[2].second};
        LOG_DEBUG("PN: '{}' PV: '{}'", pkg_n, pkg_v);
        LOG_DEBUG("Doing bash for {}", path_to_ebuild.path());
        auto temp_folder_path_res
            = co_await bash_ebuild(ioc, cfg, path_to_ebuild.path(), pkg_v);

        if (!temp_folder_path_res)
            {
                co_return std::unexpected(
                    fmt::format("Failed to run ebuild.sh: {}",
                                temp_folder_path_res.error()));
            }
        std::filesystem::path temp_file_path
            = (temp_folder_path_res.value() / "environment");

        LOG_DEBUG("Attempt to read the environment file into memory at {}",
                  temp_file_path);
        auto str_file_res = co_await file_to_string(ioc, temp_file_path);
        if (not str_file_res)
            {
                co_return std::unexpected(
                    fmt::format("Failed to read file. error: {}",
                                str_file_res.error().message()));
            }
        LOG_TRACE_L2("env_file: \n{}\n\n", str_file_res.value());

        std::string_view commit_env_var_name;

        std::string_view commit;
        if (PackageType::Commit == pkg_type)
            {
                if (not RE2::PartialMatch(str_file_res.value(),
                                          common_ctx.re_commit_str,
                                          &commit_env_var_name, &commit))
                    {
                        co_return std::unexpected(
                            fmt::format("Regex for getting commit failed, "
                                        "possible error: '{}'",
                                        common_ctx.re_commit_str.error()));
                    };
            }

        // get first url in SRC_URI
        LOG_DEBUG("Attempt to get first URL from SRC_URI from {}",
                  temp_file_path);
        std::string_view src_uri_str;

        if (not RE2::PartialMatch(str_file_res.value(), common_ctx.re_src_uri,
                                  &src_uri_str))
            {
                co_return std::unexpected(fmt::format(
                    "Regex for getting first URL from SRC_URI failed: {}",
                    common_ctx.re_src_uri.error()));
            }

        // understand what service is this
        LOG_DEBUG("Trying to undertand which service is that for {}",
                  src_uri_str);
        std::vector<int> indeces_matched;
        indeces_matched.reserve(std::size(ServicesNames));

        common_ctx.re_set_services.Match(src_uri_str, &indeces_matched);

        if (indeces_matched.empty())
            {
                co_return std::unexpected(fmt::format(
                    "Failed to understand what service is an URI. URI: {}",
                    src_uri_str));
            }

        Service service_id = magic_enum::enum_cast<Service>(
                                 static_cast<size_t>(indeces_matched[0]))
                                 .value();

        LOG_DEBUG("Matched these: {}", magic_enum::enum_name(service_id));

        co_return EbuildSpecificData{
            .filepath = path_to_ebuild,
            .p = pkg_p,
            .pv = pkg_v,
            .pn = pkg_n,
            .category = category_str,
            .first_uri = std::string(src_uri_str),
            .commit_specific = std::invoke(
                [&]() -> std::optional<CommitSpecific>
                    {
                        if (PackageType::Commit == pkg_type)
                            {
                                return CommitSpecific{
                                    .date = date,
                                    .commit = std::string{commit}};
                            }

                        return std::nullopt;
                    }),
            .service = service_id};
    }

    [[nodiscard]] corral::Task<
        std::expected<std::variant<CommitSpecific, std::string>, std::string>>
    github_fetch_version(auto& ioc, auto& semaphores,
                         EbuildSpecificData const& ebuild_data)
    {
        std::string new_version{};
        std::string new_commit{};
        uint64_t new_date = 0;

        boost::urls::url src_uri(ebuild_data.first_uri);

        std::string workspace;
        std::string repo;

        bool is_tag = false;
        for (int i = 0; auto const& seg : src_uri.encoded_segments())
            {
                switch (i)
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
                                        goto end_loop;
                                    }
                                break;
                            }
                        default:
                            break;
                    };
                ++i;
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
                tree = parse_rss_into_tree(request_str);
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
                for (int i = 0;
                     auto const& seg : parsed_link.encoded_segments())
                    {
                        LOG_TRACE_L1("{} seg: {}", i, std::string_view{seg});
                        if (3 == i && ebuild_data.commit_specific.has_value())
                            {
                                commit_version_or_tag = seg;
                            }
                        if (4 == i)
                            {
                                commit_version_or_tag = seg;
                            }
                        ++i;
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

                        std::istringstream is{date_str};

                        if (is_tag or ebuild_data.commit_specific.has_value())
                            {
                                is >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ",
                                                         time_from_epoch);
                            }
                        else
                            {
                                is >> std::chrono::parse("%Y-%m-%dT%H:%M:%S%Ez",
                                                         time_from_epoch);
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

                        new_date = static_cast<uint64_t>(
                                       (static_cast<int>(ymd.year()) * 10000))
                                   + (static_cast<uint64_t>(
                                          static_cast<unsigned>(ymd.month()))
                                      * 100)
                                   + static_cast<uint64_t>(
                                       static_cast<unsigned>(ymd.day()));
                    }
                // do it only once to get info only for latest entry.
                break;
            }
        if (ebuild_data.commit_specific.has_value())
            {
                LOG_INFO("Fetched date: {} commit: {}", new_date, new_commit);
                co_return CommitSpecific{.date = new_date,
                                         .commit = new_commit};
            }
        LOG_INFO("Fetched version '{}'", new_version);
        if (new_version.starts_with("v"))
            {
                new_version = new_version.substr(1);
            }
        co_return new_version;
    }

    // check if we support the service
    // semaphore
    // check for update
    [[nodiscard]] corral::Task<
        std::expected<std::variant<CommitSpecific, std::string>, std::string>>
    get_latest_info(auto& ioc, Config const& cfg, auto& semaphores,
                    CommonContext& common_ctx, EbuildSpecificData& ebuild_data)
    {
        // what is url of a feed for the service and the package? is it per tag,
        // per commit or per release? get feed, limit via semaphores write a
        // 27 different handlers. auto lock = co_await semaphores
        //                 .at(static_cast<size_t>(indeces_matched[0]))
        //                 .lock();

        std::variant<CommitSpecific, std::string> fetched_data{};

        switch (ebuild_data.service)
            {
                case Service::github:
                    {
                        auto fetched_res = co_await github_fetch_version(
                            ioc, semaphores, ebuild_data);
                        if (not fetched_res)
                            {
                                co_return std::unexpected(fetched_res.error());
                            }
                        fetched_data = std::move(fetched_res.value());
                        break;
                    }
                case Service::gitlab:
                case Service::bitbucket:
                case Service::codeberg:
                case Service::cpan:
                case Service::cpan_module:
                case Service::cpe:
                case Service::cran:
                case Service::ctan:
                case Service::freedesktop_gitlab:
                case Service::gentoo:
                case Service::gnome_gitlab:
                case Service::google_code:
                case Service::hackage:
                case Service::heptapod:
                case Service::kde_invent:
                case Service::launchpad:
                case Service::osdn:
                case Service::pear:
                case Service::pecl:
                case Service::pypi:
                case Service::rubygems:
                case Service::savannah:
                case Service::savannah_nongnu:
                case Service::sourceforge:
                case Service::sourcehut:
                case Service::vim:
                    co_return std::unexpected(fmt::format(
                        "This service is not yet supported: {}",
                        magic_enum::enum_name(ebuild_data.service)));
                    break;
            };

        fetched_data.visit(overloads{
            [&](CommitSpecific const& fetched) mutable
                { LOG_DEBUG("Fetched: {} {}", fetched.date, fetched.commit); },
            [&](std::string const& fetched)
                { LOG_DEBUG("Fetched: {}", fetched); }});
        co_return fetched_data;
    }

    struct EditCommit
    {
        CommitSpecific old_ver;
        CommitSpecific new_ver;
    };

    struct EditVerOrTag
    {
        std::string old_ver;
        std::string new_ver;
    };

    struct InfoForDiff
    {
        std::filesystem::path path_to_ebuild;
        std::variant<EditCommit, EditVerOrTag> data_for_how_to_change;
    };

/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) */
#define TRY_OR_CO_RETURN(expr)                                      \
    __extension__({                                                 \
        auto&& _res = (expr);                                       \
        if (!_res)                                                  \
            {                                                       \
                co_return std::unexpected(std::move(_res.error())); \
            }                                                       \
        std::move(*_res);                                           \
    })

    //  return what to change
    [[nodiscard]] corral::Task<
        std::expected<std::optional<InfoForDiff>, std::string>>
    logic_per_ebuild(auto& ioc, Config const& cfg,
                     std::filesystem::directory_entry const& path_to_ebuild,
                     auto& semaphores, CommonContext& common_ctx)
    {
        EbuildSpecificData ebuild_data = TRY_OR_CO_RETURN(
            co_await get_ebuild_info(ioc, cfg, common_ctx, path_to_ebuild));

        LOG_TRACE_L1("Current data: '{}' '{}' '{}' '{}' '{}' '{}'",
                     ebuild_data.first_uri,
                     magic_enum::enum_name(ebuild_data.service),
                     ebuild_data.filepath, ebuild_data.p, ebuild_data.pn,
                     ebuild_data.pv);

        if (ebuild_data.commit_specific.has_value())
            {
                LOG_DEBUG("Current date: '{}' commit: '{}'",
                          ebuild_data.commit_specific.value().date,
                          ebuild_data.commit_specific.value().commit);
            }

        std::variant<CommitSpecific, std::string> fetched_ver
            = TRY_OR_CO_RETURN(co_await get_latest_info(
                ioc, cfg, semaphores, common_ctx, ebuild_data));

        LOG_DEBUG("Current ver: {}", ebuild_data.pv);

        if (ebuild_data.commit_specific.has_value())
            {
                LOG_INFO("Current date: {} commit: {}",
                         ebuild_data.commit_specific.value().date,
                         ebuild_data.commit_specific.value().commit);
            }

        fetched_ver.visit(overloads{
            [&](CommitSpecific const& fetched) mutable
                {
                    LOG_INFO("Fetched: '{}' '{}' '{}'", ebuild_data.p,
                             fetched.date, fetched.commit);
                },
            [&](std::string const& fetched)
                { LOG_INFO("Fetched: '{}' '{}'", ebuild_data.p, fetched); }});

        bool is_changed = false;
        fetched_ver.visit(overloads{
            [&](CommitSpecific const& fetched) mutable
                {
                    if (ebuild_data.commit_specific.value().date < fetched.date)
                        {
                            is_changed = true;
                        }
                },
            [&](std::string const& fetched) mutable
                {
                    if (ebuild_data.pv < fetched)
                        {
                            is_changed = true;
                        }
                }});

        if (is_changed)
            {
                LOG_INFO("New version!!!");
                InfoForDiff diff{};
                fetched_ver.visit(overloads{
                    [&](CommitSpecific const& fetched) mutable
                        {
                            diff = {.data_for_how_to_change = EditCommit{
                                        .old_ver
                                        = ebuild_data.commit_specific.value(),
                                        .new_ver = fetched}};
                        },
                    [&](std::string const& fetched) mutable
                        {
                            diff = {.data_for_how_to_change
                                    = EditVerOrTag{.old_ver = ebuild_data.pv,
                                                   .new_ver = fetched}};
                        }});
                diff.path_to_ebuild = ebuild_data.filepath;
                co_return diff;
            }

        // return what has to be changed

        co_return std::nullopt;
    }


    [[nodiscard]] corral::Task<ReturnCode> chief_logic(auto& ioc,
                                                       Config const& cfg,
                                                       auto& semaphores)
    {
        // to compile them all at once
        // NOLINTBEGIN(hicpp-signed-bitwise)
        std::string str_re_versions = reflex::PCRE2UTFMatcher::convert(
            R"(([\w][\w+-]*?)-((\d+)(\.\d+)*)([a-z]?)((_(pre|p|beta|alpha|rc)\d*)*)(-r(\d+))?)",
            reflex::convert_flag::unicode | reflex::convert_flag::notnewline);
        // NOLINTEND(hicpp-signed-bitwise)

        const reflex::PCRE2UTFMatcher::Pattern& pattern_re_versions(
            str_re_versions);

        CommonContext common_ctx{
            .re_commit_str = RE2(
                R"delimiter(declare -- ([a-zA-Z_]?[a-zA-Z0-9_]*?COMMIT[a-zA-Z0-9_]*?)="([0-9a-f]{40})"\n)delimiter",
                RE2::Quiet),
            .re_src_uri = RE2(
                R"delimiter(declare SRC_URI=\$?["'](?:\\n)?(?:\\t)?(?:\s*)?(https?://\S*).*?['"])delimiter",
                RE2::Quiet),

            .re_category = RE2(R"(([\w][\w+.-]*))", RE2::Quiet),
            .re_pkg_9999 = RE2(R"([\w+.-]*9999)", RE2::Quiet),

            .re_pkg_with_date = RE2(R"([\w+.-]+?(\d{8})[\w+.-]*?)", RE2::Quiet),
            .re_set_services = std::invoke(
                []()
                    {
                        RE2::Set re_set_services(RE2::DefaultOptions,
                                                 RE2::Anchor::UNANCHORED);
                        for (auto&& service : ServicesNames)
                            {
                                re_set_services.Add(service, nullptr);
                            }
                        re_set_services.Compile();
                        return re_set_services;
                    }),
            .re_version_matcher = reflex::PCRE2UTFMatcher(pattern_re_versions),
        };

        std::vector<InfoForDiff> what_to_change;
        bool is_any_successful = false;
        bool is_any_failed = false;

        CORRAL_WITH_NURSERY(nursery)
        {
            for (const auto& category :
                 std::filesystem::directory_iterator(cfg.path_to_repo))
                {
                    if (not category.is_directory())
                        {
                            continue;
                        }
                    std::string category_str
                        = category.path().filename().string();
                    if (not RE2::FullMatch(category_str,
                                           common_ctx.re_category))
                        {
                            continue;
                        }

                    for (const auto& pkg_name :
                         std::filesystem::directory_iterator(category))
                        {
                            if (not pkg_name.is_directory())
                                {
                                    continue;
                                }

                            for (const auto& file :
                                 std::filesystem::directory_iterator(pkg_name))
                                {
                                    if (not file.is_regular_file())
                                        {
                                            continue;
                                        }

                                    std::filesystem::path pkg_filename
                                        = file.path().filename();

                                    if (pkg_filename.extension() != ".ebuild")
                                        {
                                            continue;
                                        }

                                    std::string pkg_p
                                        = pkg_filename.stem().string();

                                    if (RE2::FullMatch(pkg_p,
                                                       common_ctx.re_pkg_9999))
                                        {
                                            continue;
                                        }
                                    nursery.start(
                                        [&](std::filesystem::directory_entry
                                                file_arg) mutable
                                            -> corral::Task<void>
                                            {
                                                auto sth
                                                    = co_await logic_per_ebuild(
                                                        ioc, cfg, file_arg,
                                                        semaphores, common_ctx);
                                                if (not sth)
                                                    {
                                                        is_any_failed = true;
                                                        LOG_ERROR(
                                                            "Failed to do "
                                                            "sth "
                                                            "with a "
                                                            "ebuild: {}",
                                                            sth.error());
                                                        co_return;
                                                    }
                                                if (not sth.value().has_value())
                                                    {
                                                        is_any_successful
                                                            = true;
                                                        LOG_INFO(
                                                            "Nothing to change "
                                                            "for {}",
                                                            file_arg.path());
                                                        co_return;
                                                    }
                                                what_to_change.emplace_back(
                                                    sth.value().value());
                                            },
                                        file);
                                }
                        }
                }

            co_return corral::join;
        };

        for (auto& diff : what_to_change)
            {
                if (std::holds_alternative<EditCommit>(
                        diff.data_for_how_to_change))
                    {
                        EditCommit commits_diff
                            = std::get<EditCommit>(diff.data_for_how_to_change);
                        LOG_INFO(
                            "Edit: path: '{}' old date: '{}' new date: '{}' "
                            "old commit: '{}' new commit: '{}'",
                            diff.path_to_ebuild, commits_diff.old_ver.date,
                            commits_diff.new_ver.date,
                            commits_diff.old_ver.commit,
                            commits_diff.new_ver.commit);
                    }
                else if (std::holds_alternative<EditVerOrTag>(
                             diff.data_for_how_to_change))
                    {
                        EditVerOrTag commits_diff = std::get<EditVerOrTag>(
                            diff.data_for_how_to_change);
                        LOG_INFO("Edit: path: '{}' old ver: '{}' new ver: '{}'",
                                 diff.path_to_ebuild, commits_diff.old_ver,
                                 commits_diff.new_ver);
                    }
            }

        if (is_any_failed and is_any_successful)
            {
                co_return ReturnCode::PartialSuccess;
            }
        else if (is_any_failed and not is_any_successful)
            {
                co_return ReturnCode::AllHaveFailed;
            }
        else if (not is_any_successful)
            {
                co_return ReturnCode::NoEbuildFound;
            }
        else
            {
                co_return ReturnCode::Success;
                ;
            }
    }

    corral::Task<ReturnCode> actual_chief(auto& ioc, Config const& cfg)
    {
        constexpr size_t amount_of_services = magic_enum::enum_count<Service>();

        std::array<corral::Semaphore, amount_of_services>
            semaphores_per_services
            = make_array_from_factory<amount_of_services>(
                [concurrency_per_service = cfg.concurrency_per_service]()
                    { return corral::Semaphore(concurrency_per_service); });

        ReturnCode res
            = co_await chief_logic(ioc, cfg, semaphores_per_services);
        co_return res;
    }

}  // namespace

int main(int argc, char* argv[])
{
    Config cfg;

    // Config parsing
    {
        CLI::App app{
            "Post-processor for YouTube's RSS feed, so that you get "
            "summary of video inside the feed via sending an HTTP "
            "request to something like an Ollama instance."};

        // Temporary storage for CLI11 to map types it doesn't handle
        // natively without custom validators
        std::vector<std::string> headers_raw
            = {"Content-Type: application/json"};
        std::string log_level_str = "info";

        app.add_option("-H,--header", headers_raw,
                       "HTTP headers for request to an ?Ollama? instance.")
            ->take_all();

        app.add_option("-l,--log-file", cfg.log_file,
                       "Filepath to internal logs")
            ->default_val("./logs.log");

        app.add_option("-R,--repo-path", cfg.path_to_repo, "Filepath to repo")
            ->required();

        app.add_option("-t,--tmp-path", cfg.path_to_portage_temp,
                       "Filepath to portage's tmp")
            ->required();

        app.add_option("-G,--gentoo-repo-path", cfg.path_to_gentoo_repo,
                       "Filepath to gentoo's repo path")
            ->default_val("/var/db/repos/gentoo/");

        app.add_option("-P,--portage-bin-path", cfg.path_to_portage_bin,
                       "Filepath to portage's bin folder i.e. `git clone "
                       "https://github.com/Arniiiii/portage` and give the path "
                       "to `./bin/` folder")
            ->required();

        app.add_option("-Y,--portage-pym-path", cfg.path_to_portage_pym,
                       "Filepath to portage's PYM folder. I have no idea what "
                       "this is, but it can be any empty folder. ")
            ->required();

        app.add_option("--log-level", log_level_str,
                       "Log level: "
                       "tracel3,tracel2,tracel1,debug,info,notice,warning,"
                       "error,critical")
            ->default_val("info");

        app.add_option("-J,--jobs-requests", cfg.concurrency_per_service,
                       "Amount of concurrent request per service"
                       "by this application")
            ->check(CLI::PositiveNumber)
            ->default_val(DEFAULT_MAX_CONCURRENT_REQUESTS_PER_SERVICE);
        try
            {
                app.parse(argc, argv);

                cfg.log_level = quill::loglevel_from_string(log_level_str);

                for (const auto& header_raw_str : headers_raw)
                    {
                        std::vector<std::string> parts;
                        boost::split(parts, header_raw_str,
                                     boost::is_any_of(":"));
                        if (parts.size() >= 2)
                            {
                                std::string key = boost::trim_copy(parts[0]);
                                std::string val = boost::trim_copy(parts[1]);
                                cfg.headers.insert(key, val);
                            }
                    }
            }
        catch (const CLI::ParseError& e)
            {
                return app.exit(e);
            }
        catch (OmegaException<std::string>& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                std::string log = fmt::format(
                    "Oohh, look at you, who got an exception, my cutie "
                    "lovely "
                    "guy. "
                    "\nThis is an Omega exception. Most definitely an "
                    "incorrect "
                    "value was specified in args. Here's "
                    ".what():\n\n{}\nHere's data:\n{}\n Here's "
                    "trace:\n{}\nHere's "
                    "line where something failed:\n{}\n\nHere's an attempt "
                    "to "
                    "get "
                    "backtrace of it "
                    "via "
                    "boost::stacktrace and libbacktrace...",
                    e.what(), e.data(), e.stack(), e.where(),
                    boost::stacktrace::to_string(trace));

                fmt::print(std::cerr, "{}", log);
                return std::to_underlying(
                    ReturnCode::FailSpecifiedValueIsIncorrect);
            }
        catch (std::exception& e)
            {
                boost::stacktrace::basic_stacktrace<
                    std::allocator<boost::stacktrace::frame>>
                    trace
                    = boost::stacktrace::stacktrace::from_current_exception();
                fmt::print(std::cerr,
                           "Oohh, look at you, who got an exception, my cutie "
                           "lovely guy. "
                           "\nYou know that standard exceptions sucks. Here's "
                           ".what():\n\n{}\n\nHere's an attempt to get "
                           "backtrace of it "
                           "via "
                           "boost::stacktrace and libbacktrace... Hope it "
                           "works or kill "
                           "youself debugging this shit.\n{}\n",
                           e.what(), boost::stacktrace::to_string(trace));
                return std::to_underlying(
                    ReturnCode::FailDuringInitializationConfig);
            }
    }

    // Enabling logging
    try
        {
            setup_quill(cfg.log_file, cfg.log_level);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            fmt::print(std::cerr,
                       "Oohh, look at you, who got an exception, my cutie "
                       "lovely guy. "
                       "\nYou know that standard exceptions sucks. Here's "
                       ".what():\n\n{}\n\nHere's an attempt to get "
                       "backtrace of it "
                       "via "
                       "boost::stacktrace and libbacktrace... Hope it "
                       "works or kill "
                       "youself debugging this shit.\n{}\n",
                       e.what(), boost::stacktrace::to_string(trace));
            return std::to_underlying(ReturnCode::FailInitializationLogger);
        }

    // Finally main.
    try
        {
            boost::asio::io_context ioc;
            LOG_DEBUG("Entering asynchronous main...");
            boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
            auto&& [main_opt, signals_opt] = corral::run(
                ioc, corral::anyOf(actual_chief(ioc, cfg),
                                   signals.async_wait(corral::asio_awaitable)));
            if (main_opt)
                {
                    return std::to_underlying(main_opt.value());
                }
            return std::to_underlying(ReturnCode::ReceivedCancellationSignal);
        }
    catch (OmegaException<std::string>& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely "
                "guy. "
                "\nThis is an Omega exception. Maybe from parsing result "
                "from "
                "Ollama. Here's "
                ".what():\n\n{}\nHere's data:\n{}\n Here's "
                "trace:\n{}\nHere's "
                "line where something failed:\n{}\n\nHere's an attempt to "
                "get "
                "backtrace of it "
                "via "
                "boost::stacktrace and libbacktrace...",
                e.what(), e.data(), e.stack(), e.where(),
                boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(ReturnCode::FailParsePromptResult);
        }
    catch (std::exception& e)
        {
            boost::stacktrace::basic_stacktrace<
                std::allocator<boost::stacktrace::frame>>
                trace = boost::stacktrace::stacktrace::from_current_exception();
            std::string log = fmt::format(
                "Oohh, look at you, who got an exception, my cutie lovely "
                "guy. "
                "\nYou know that standard exceptions sucks. Here's "
                ".what():\n\n{}\n\nHere's an attempt to get backtrace of "
                "it "
                "via "
                "boost::stacktrace and libbacktrace... Hope it works or "
                "kill "
                "youself debugging this shit.\n{}\n",
                e.what(), boost::stacktrace::to_string(trace));

            fmt::print(std::cerr, "{}", log);
            LOG_ERROR("{}", log);
            return std::to_underlying(ReturnCode::FailStandardException);
        }
}
