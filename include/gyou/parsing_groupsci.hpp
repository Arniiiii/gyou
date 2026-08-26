#ifndef INCLUDE_GYOU_PARSING_GROUPSCI_HPP_
#define INCLUDE_GYOU_PARSING_GROUPSCI_HPP_

#include <expected>
#include <string>
#include <unordered_map>

namespace gyou
{
    struct GroupsCollection
    {
        std::size_t amount_of_groups;
        std::unordered_map<std::string, size_t> groups;
    };
    std::expected<GroupsCollection, std::string> parse_groups(
        std::string const& groups_str);
}  // namespace gyou

#endif  // INCLUDE_GYOU_PARSING_GROUPSCI_HPP_
