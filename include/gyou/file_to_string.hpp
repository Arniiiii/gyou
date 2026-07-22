#ifndef INCLUDE_GYOU_FILE_TO_STRING_HPP_
#define INCLUDE_GYOU_FILE_TO_STRING_HPP_

#define BOOST_ASIO_HAS_FILE 1
#define BOOST_ASIO_HAS_IO_URING 1

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/basic_stream_file.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/date_time.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <corral/detail/asio.h>

#include "gyou/utils/omega_exception.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<
        std::expected<std::string, boost::system::error_code>>
    file_to_string(auto& ioc, std::filesystem::path const& file_path)
    {
        if (not std::filesystem::exists(file_path))
            {
                throw OmegaException<std::filesystem::path>(
                    "It was requested to read a file into a string on heap "
                    "asynchronously, but the file just simply does not exist.",
                    file_path);
            }
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
}  // namespace gyou

#endif  // INCLUDE_GYOU_FILE_TO_STRING_HPP_
