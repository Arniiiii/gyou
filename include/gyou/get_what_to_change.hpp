#ifndef INCLUDE_GYOU_GET_WHAT_TO_CHANGE_HPP_
#define INCLUDE_GYOU_GET_WHAT_TO_CHANGE_HPP_

#include <boost/asio/io_context.hpp>
#include <corral/corral.h>

#include "gyou/semaphore_array_type.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/result_of_parsing.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<gyou::PackagesToUpdate> get_what_to_change(
        boost::asio::io_context& ioc, gyou::Config const& cfg,
        gyou::semaphores_array_type& semaphores,
        gyou::CommonContext& common_ctx);

}  // namespace gyou

#endif  // INCLUDE_GYOU_GET_WHAT_TO_CHANGE_HPP_
