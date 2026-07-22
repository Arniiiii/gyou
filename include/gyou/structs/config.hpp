#ifndef INCLUDE_STRUCTS_CONFIG_HPP_
#define INCLUDE_STRUCTS_CONFIG_HPP_

#include <filesystem>

#include <boost/beast/http/fields.hpp>
#include <quill/core/LogLevel.h>

namespace gyou
{

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct Config
    {
        boost::beast::http::fields headers;
        std::filesystem::path log_file;
        std::filesystem::path path_to_repo;
        std::filesystem::path path_to_portage_bin;
        std::filesystem::path path_to_tmp;
        std::filesystem::path path_to_portage_pym;
        std::filesystem::path path_to_gentoo_repo;
        std::string main_branch_name;
        size_t concurrency_per_service{};
        quill::LogLevel log_level{};
    };
}  // namespace gyou

#endif  // INCLUDE_STRUCTS_CONFIG_HPP_
