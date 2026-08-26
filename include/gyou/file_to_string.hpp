#ifndef INCLUDE_GYOU_FILE_TO_STRING_HPP_
#define INCLUDE_GYOU_FILE_TO_STRING_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

namespace gyou
{

    [[nodiscard]] corral::Task<
        std::expected<std::string, boost::system::error_code>>
    file_to_string(boost::asio::io_context& ioc,
                   std::filesystem::path const& file_path);
}  // namespace gyou

#endif  // INCLUDE_GYOU_FILE_TO_STRING_HPP_
