#ifndef INCLUDE_STRUCTS_RESULT_OF_PARSING_HPP_
#define INCLUDE_STRUCTS_RESULT_OF_PARSING_HPP_

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include "gyou/structs/change_related/commit_specific.hpp"

namespace gyou
{

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EditCommit
    {
        std::string env_var_name;
        gyou::CommitSpecific old_ver;
        gyou::CommitSpecific new_ver;
    };

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EditVerOrTag
    {
        std::string old_ver;
        std::string new_ver;
    };

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct InfoForDiff
    {
        std::filesystem::path path_to_ebuild;
        std::variant<EditCommit, EditVerOrTag> data_for_how_to_change;
    };

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct PackagesToUpdate
    {
        std::vector<InfoForDiff> what_to_change;
        bool is_any_successful = false;
        bool is_any_failed = false;
    };
}  // namespace gyou

#endif  // INCLUDE_STRUCTS_RESULT_OF_PARSING_HPP_
