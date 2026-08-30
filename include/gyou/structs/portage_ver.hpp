#ifndef INCLUDE_STRUCTS_PORTAGE_VER_HPP_
#define INCLUDE_STRUCTS_PORTAGE_VER_HPP_

#include <string>
namespace gyou
{
    struct ParsedEbuildStem
    {
        std::string pn;
        std::string pn_inval;
        std::string ver;
        std::string rev;
    };

}  // namespace gyou

#endif  // INCLUDE_STRUCTS_PORTAGE_VER_HPP_
