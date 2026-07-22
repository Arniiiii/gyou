#ifndef INCLUDE_STRUCTS_SERVICE_ENUM_HPP_
#define INCLUDE_STRUCTS_SERVICE_ENUM_HPP_

#include <cstddef>
namespace gyou
{

    // NOLINTNEXTLINE(performance-enum-size)
    enum class Service : std::size_t
    {
        github = 0,
        gitlab = 1,
        bitbucket = 2,
        codeberg = 3,
        cpan = 4,
        cpan_module = 5,
        cpe = 6,
        cran = 7,
        ctan = 8,
        freedesktop_gitlab = 9,
        gentoo = 10,
        gnome_gitlab = 11,
        google_code = 12,
        hackage = 13,
        heptapod = 14,
        kde_invent = 15,
        launchpad = 16,
        osdn = 17,
        pear = 18,
        pecl = 19,
        pypi = 20,
        rubygems = 21,
        savannah = 22,
        savannah_nongnu = 23,
        sourceforge = 24,
        sourcehut = 25,
        vim = 26,
    };
}  // namespace gyou

#endif  // INCLUDE_STRUCTS_SERVICE_ENUM_HPP_
