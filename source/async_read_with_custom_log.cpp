#include "gyou/async_read_with_custom_log.hpp"

#include <array>
#include <string>

#include <boost/asio/readable_pipe.hpp>
#include <boost/date_time.hpp>
#include <corral/Task.h>
#include <corral/asio.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>

#include "quill_usage/global_logger_ptr.hpp"

namespace gyou
{

    corral::Task<std::string> read_loop(std::string const& logger_name,
                                        boost::asio::readable_pipe& a_pipe)
    {
        std::string res;
        std::array<char, 4096> buf;
        quill::Logger* logger_with_custom_name
            = quill::Frontend::create_or_get_logger(logger_name,
                                                    global_logger_a);
        logger_with_custom_name->set_log_level(
            global_logger_a->get_log_level());

        for (;;)
            {
                auto [error_code, received_size]
                    = co_await a_pipe.async_read_some(
                        boost::asio::buffer(buf),
                        corral::asio_nothrow_awaitable);
                if (received_size != 0U)
                    {
                        QUILL_LOG_TRACE_L2(
                            logger_with_custom_name, "{}",
                            std::string_view(buf.data(), received_size));
                        res.append(buf.data(), received_size);
                    }
                if (error_code)
                    {
                        co_return res;
                    }
            }
    };

}  // namespace gyou
