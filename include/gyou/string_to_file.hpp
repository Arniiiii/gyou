#ifndef INCLUDE_GYOU_STRING_TO_FILE_HPP_
#define INCLUDE_GYOU_STRING_TO_FILE_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/system/detail/error_code.hpp>
#include <corral/Task.h>

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, boost::system::error_code>>
    string_to_file(boost::asio::io_context& ioc, std::string const& content,
                   std::filesystem::path const& file_path);
}  // namespace gyou

#endif  // INCLUDE_GYOU_STRING_TO_FILE_HPP_
