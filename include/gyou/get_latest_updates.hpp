#ifndef INCLUDE_GYOU_GET_LATEST_UPDATES_HPP_
#define INCLUDE_GYOU_GET_LATEST_UPDATES_HPP_

#include <expected>
#include <string>
#include <variant>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/semaphore_array_type.hpp"
#include "gyou/structs/change_related/commit_specific.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"

namespace gyou
{
    // check if we support the service
    // semaphore
    // check for update
    [[nodiscard]] corral::Task<std::expected<
        std::variant<gyou::CommitSpecific, std::string>, std::string>>
    get_latest_info(boost::asio::io_context& ioc, gyou::Config const& cfg,
                    gyou::semaphores_array_type& semaphores,
                    gyou::CommonContext& common_ctx,
                    gyou::EbuildSpecificData const& ebuild_data);

}  // namespace gyou

#endif  // INCLUDE_GYOU_GET_LATEST_UPDATES_HPP_
