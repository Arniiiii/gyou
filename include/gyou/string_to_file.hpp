#ifndef INCLUDE_GYOU_STRING_TO_FILE_HPP_
#define INCLUDE_GYOU_STRING_TO_FILE_HPP_

#include <boost/asio/basic_stream_file.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/date_time.hpp>
#include <corral/asio.h>
#include <corral/corral.h>
#include <corral/detail/asio.h>

#include "gyou/utils/omega_exception.hpp"
namespace gyou {

    [[nodiscard]] corral::Task<std::expected<void, boost::system::error_code>>
    string_to_file(auto& ioc, std::string const& content,
                   std::filesystem::path const& file_path)
    {
        if (not std::filesystem::exists(file_path))
            {
                throw OmegaException<std::filesystem::path>(
                    "It was requested to write a string into a file "
                    "asynchronously, but the file already exists. Refusing to "
                    "rewrite a file.",
                    file_path);
            }
        boost::asio::stream_file file_writer(
            ioc, file_path, boost::asio::stream_file::write_only);

        auto&& [errc, bytes_read] = co_await boost::asio::async_write(
            file_writer, boost::asio::buffer(content),
            corral::asio_nothrow_awaitable);
        if (errc && boost::asio::error::eof != errc)
            {
                co_return std::unexpected(errc);
            }
        co_return {};
    }
}

#endif  // INCLUDE_GYOU_STRING_TO_FILE_HPP_
