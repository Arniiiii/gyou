#ifndef INCLUDE_GYOU_GET_LATEST_UPDATES_HPP_
#define INCLUDE_GYOU_GET_LATEST_UPDATES_HPP_

#include <expected>
#include <string>
#include <variant>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>
#include <boost/process/v2/environment.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "gyou/fetchers/github.hpp"
#include "gyou/structs/change_related/commit_specific.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"
#include "gyou/utils/variants_utils.hpp"
#include "overwrite_log_macros.hpp"

namespace gyou
{
    // check if we support the service
    // semaphore
    // check for update
    [[nodiscard]] corral::Task<std::expected<
        std::variant<gyou::CommitSpecific, std::string>, std::string>>
    get_latest_info(auto& ioc, gyou::Config const& cfg, auto& semaphores,
                    gyou::CommonContext& common_ctx,
                    gyou::EbuildSpecificData const& ebuild_data)
    {
        // what is url of a feed for the service and the package? is it per tag,
        // per commit or per release? get feed, limit via semaphores write a
        // 27 different handlers. auto lock = co_await semaphores
        //                 .at(static_cast<size_t>(indeces_matched[0]))
        //                 .lock();

        std::variant<gyou::CommitSpecific, std::string> fetched_data{};

        switch (ebuild_data.service)
            {
                case gyou::Service::github:
                    {
                        auto fetched_res = co_await gyou::github_fetch_version(
                            ioc, semaphores, ebuild_data);
                        if (not fetched_res)
                            {
                                co_return std::unexpected(fetched_res.error());
                            }
                        fetched_data = std::move(fetched_res.value());
                        break;
                    }
                case gyou::Service::gitlab:
                case gyou::Service::bitbucket:
                case gyou::Service::codeberg:
                case gyou::Service::cpan:
                case gyou::Service::cpan_module:
                case gyou::Service::cpe:
                case gyou::Service::cran:
                case gyou::Service::ctan:
                case gyou::Service::freedesktop_gitlab:
                case gyou::Service::gentoo:
                case gyou::Service::gnome_gitlab:
                case gyou::Service::google_code:
                case gyou::Service::hackage:
                case gyou::Service::heptapod:
                case gyou::Service::kde_invent:
                case gyou::Service::launchpad:
                case gyou::Service::osdn:
                case gyou::Service::pear:
                case gyou::Service::pecl:
                case gyou::Service::pypi:
                case gyou::Service::rubygems:
                case gyou::Service::savannah:
                case gyou::Service::savannah_nongnu:
                case gyou::Service::sourceforge:
                case gyou::Service::sourcehut:
                case gyou::Service::vim:
                    co_return std::unexpected(fmt::format(
                        "This service is not yet supported: {}",
                        magic_enum::enum_name(ebuild_data.service)));
                    break;
            };

        fetched_data.visit(
            overloads{[&](gyou::CommitSpecific const& fetched) mutable
                          {
                              LOG_DEBUG("Fetched: '{}' '{}'", fetched.date,
                                        fetched.commit);
                          },
                      [&](std::string const& fetched)
                          { LOG_DEBUG("Fetched: {}", fetched); }});
        co_return fetched_data;
    }

}  // namespace gyou

#endif  // INCLUDE_GYOU_GET_LATEST_UPDATES_HPP_
