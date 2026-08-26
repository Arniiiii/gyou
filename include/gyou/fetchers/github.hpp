#ifndef INCLUDE_GYOU_GITHUB_FETCH_VER_HPP_
#define INCLUDE_GYOU_GITHUB_FETCH_VER_HPP_

#include <expected>
#include <string>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/semaphore_array_type.hpp"
#include "gyou/structs/change_related/commit_specific.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"

namespace gyou
{
    [[nodiscard]] corral::Task<std::expected<
        std::variant<gyou::CommitSpecific, std::string>, std::string>>
    github_fetch_version(boost::asio::io_context& ioc,
                         gyou::semaphores_array_type& semaphores,
                         gyou::EbuildSpecificData const& ebuild_data,
                         gyou::CommonContext& common_ctx);

}  // namespace gyou

#endif  // INCLUDE_GYOU_GITHUB_FETCH_VER_HPP_
