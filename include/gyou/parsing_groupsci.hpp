#ifndef INCLUDE_GYOU_PARSING_GROUPSCI_HPP_
#define INCLUDE_GYOU_PARSING_GROUPSCI_HPP_

#include <string>
#include <vector>
#include <unordered_map>

#include <glaze/glaze.hpp>
#include <glaze/json.hpp>

namespace gyou
{
    struct GroupsCollection
    {
        std::size_t amount_of_groups;
        std::unordered_map<std::string, size_t> groups;
    };
    inline std::expected<GroupsCollection, std::string> parse_groups(
        std::string const& groups_str)
    {
        std::expected<std::vector<std::vector<std::string>>, glz::error_ctx> sth
            = glz::read_json<std::vector<std::vector<std::string>>>(groups_str);
        return sth
            .transform_error(
                [&](glz::error_ctx& sth_err)
                    { return glz::format_error(sth_err, groups_str); })
            .and_then(
                [](std::vector<std::vector<std::string>>&&
                       collection_of_groups_of_ebuild_pv)
                    -> std::expected<GroupsCollection, std::string>
                    {
                        std::unordered_map<std::string, size_t> res;
                        res.reserve(__extension__({
                            size_t size_ = 0;
                            for (auto const& group :
                                 collection_of_groups_of_ebuild_pv)
                                {
                                    size_ += std::size(group);
                                };
                            size_;
                        }));
                        size_t group_num = 1;
                        for (auto&& group : collection_of_groups_of_ebuild_pv)
                            {
                                for (auto&& pv : group)
                                    {
                                        res.insert_or_assign(std::move(pv),
                                                             group_num);
                                    }
                                ++group_num;
                            }
                        return GroupsCollection{
                            .amount_of_groups = group_num - 1,
                            .groups = std::move(res)};
                    });
    }
}  // namespace gyou

#endif  // INCLUDE_GYOU_PARSING_GROUPSCI_HPP_
