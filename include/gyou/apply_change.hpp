#ifndef INCLUDE_GYOU_APPLY_CHANGE_HPP_
#define INCLUDE_GYOU_APPLY_CHANGE_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/structs/config.hpp"
#include "gyou/structs/result_of_parsing.hpp"

namespace gyou
{

    [[nodiscard]] corral::Task<std::expected<void, std::string>> apply_change(
        boost::asio::io_context& ioc, gyou::Config const& cfg,
        std::filesystem::path const where_to_change,
        gyou::InfoForDiff const& diff_info);

}  // namespace gyou

#endif  // INCLUDE_GYOU_APPLY_CHANGE_HPP_
