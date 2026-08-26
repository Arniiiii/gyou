#ifndef INCLUDE_GYOU_GIT_WORKTREE_HPP_
#define INCLUDE_GYOU_GIT_WORKTREE_HPP_

#include <expected>
#include <filesystem>
#include <string>

#include <boost/asio/io_context.hpp>
#include <corral/Task.h>

#include "gyou/structs/config.hpp"

namespace gyou
{
    [[nodiscard]] corral::Task<std::expected<void, std::string>>
    git_create_worktree(boost::asio::io_context& ioc, gyou::Config const& cfg,
                        std::filesystem::path const& path_to_git,
                        std::filesystem::path const folder_path,
                        std::string const branch_name);

}  // namespace gyou

#endif  // INCLUDE_GYOU_GIT_WORKTREE_HPP_
