#ifndef INCLUDE_CHANGE_RELATED_EBUILD_COMMIT_SPECIFIC_HPP_
#define INCLUDE_CHANGE_RELATED_EBUILD_COMMIT_SPECIFIC_HPP_

#include <cstdint>
#include <string>

namespace gyou
{
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EbuildCommitSpecific
    {
        uint64_t date{};
        std::string commit;
        std::string env_var_name;
    };

}  // namespace gyou

#endif  // INCLUDE_CHANGE_RELATED_EBUILD_COMMIT_SPECIFIC_HPP_
