#ifndef INCLUDE_RSS_MOST_CASES_HPP_
#define INCLUDE_RSS_MOST_CASES_HPP_

#include <string>
namespace gyou
{

    // NOLINTNEXTLINE(altera-struct-pack-align)
    struct EntryData
    {
        std::string link_str;
        std::string author;
        std::string title;
        std::string description;
    };
}  // namespace gyou

#endif  // INCLUDE_RSS_MOST_CASES_HPP_
