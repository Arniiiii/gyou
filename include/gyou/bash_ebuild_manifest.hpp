#ifndef INCLUDE_GYOU_BASH_EBUILD_MANIFEST_HPP_
#define INCLUDE_GYOU_BASH_EBUILD_MANIFEST_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/structs/config.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, std::string>>
    ebuild_manifest_update(boost::asio::io_context& ioc,
                           gyou::Config const& cfg,
                           std::filesystem::path const& path_to_ebuild);
}  // namespace gyou

#endif  // INCLUDE_GYOU_BASH_EBUILD_MANIFEST_HPP_
