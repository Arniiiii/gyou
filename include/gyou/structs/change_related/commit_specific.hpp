#ifndef INCLUDE_CHANGE_RELATED_COMMIT_SPECIFIC_HPP_
#define INCLUDE_CHANGE_RELATED_COMMIT_SPECIFIC_HPP_

#include <cstdint>
#include <string>
namespace gyou
{
    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct CommitSpecific
    {
        uint64_t date{};
        std::string commit;
    };

}  // namespace gyou

#endif  // INCLUDE_CHANGE_RELATED_COMMIT_SPECIFIC_HPP_
