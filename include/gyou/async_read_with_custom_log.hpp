#ifndef INCLUDE_GYOU_ASYNC_READ_WITH_CUSTOM_LOG_HPP_
#define INCLUDE_GYOU_ASYNC_READ_WITH_CUSTOM_LOG_HPP_

#include <string>

#include <boost/asio/readable_pipe.hpp>
#include <corral/Task.h>

namespace gyou
{

    corral::Task<std::string> read_loop(std::string const& logger_name,
                                        boost::asio::readable_pipe& a_pipe);

}  // namespace gyou
#endif  // INCLUDE_GYOU_ASYNC_READ_WITH_CUSTOM_LOG_HPP_
