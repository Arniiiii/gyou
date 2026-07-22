#ifndef INCLUDE_STRUCTS_EBUILD_PARSED_DATA_HPP_
#define INCLUDE_STRUCTS_EBUILD_PARSED_DATA_HPP_

#include <filesystem>
#include <optional>
#include <string>

#include "gyou/structs/ebuild_commit_specific.hpp"
#include "gyou/structs/service_enum.hpp"
namespace gyou
{
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EbuildSpecificData
    {
        std::filesystem::path filepath;
        std::string p;
        std::string pv;
        std::string pn;
        std::string category;
        std::string first_uri;
        std::optional<gyou::EbuildCommitSpecific> commit_specific;
        gyou::Service service{};
    };

}  // namespace gyou

#endif  // INCLUDE_STRUCTS_EBUILD_PARSED_DATA_HPP_
