#ifndef INCLUDE_GYOU_PARSING_ONE_EBUILD_HPP_
#define INCLUDE_GYOU_PARSING_ONE_EBUILD_HPP_

#include <expected>
#include <filesystem>
#include <optional>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/semaphore_array_type.hpp"
#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/result_of_parsing.hpp"

namespace gyou
{

    //  return what to change
    [[nodiscard]] corral::Task<
        std::expected<std::optional<InfoForDiff>, std::string>>
    logic_per_ebuild(boost::asio::io_context& ioc, gyou::Config const& cfg,
                     std::filesystem::directory_entry const& path_to_ebuild,
                     gyou::semaphores_array_type& semaphores,
                     gyou::CommonContext& common_ctx);

}  // namespace gyou

#endif  // INCLUDE_GYOU_PARSING_ONE_EBUILD_HPP_
