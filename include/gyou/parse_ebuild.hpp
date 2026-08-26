#ifndef INCLUDE_GYOU_PARSE_EBUILD_HPP_
#define INCLUDE_GYOU_PARSE_EBUILD_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/structs/common_ctx.hpp"
#include "gyou/structs/config.hpp"
#include "gyou/structs/ebuild_parsed_data.hpp"

namespace gyou
{

    // get current version of the pkg or commit of
    // current pkg extract service and link
    [[nodiscard]] corral::Task<
        std::expected<gyou::EbuildSpecificData, std::string>>
    get_ebuild_info(boost::asio::io_context& ioc, gyou::Config const& cfg,
                    gyou::CommonContext& common_ctx,
                    std::filesystem::directory_entry const& path_to_ebuild);
}  // namespace gyou

#endif  // INCLUDE_GYOU_PARSE_EBUILD_HPP_
