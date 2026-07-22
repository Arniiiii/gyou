#ifndef INCLUDE_STRUCTS_SERVICE_REGEX_HPP_
#define INCLUDE_STRUCTS_SERVICE_REGEX_HPP_

#include <string_view>

#include "gyou/structs/service_enum.hpp"
#include "gyou/utils/designated_array_initializers.hpp"

namespace gyou
{

    // Fucking C++ without C99 designated array initializer extension makes me
    // do this shit.
    constexpr auto ServicesNames
        = gyou::ArrayBuilder<std::string_view>()
              .e<Service::bitbucket>("https://bitbucket.org/.*?")
              .e<Service::codeberg>("https://codeberg.org/.*?")
              .e<Service::cpan>("https://metacpan.org/dist/.*?")
              .e<Service::cpan_module>("https://metacpan.org/pod/.*?")
              .e<Service::cpe>("cpe:/.*?")
              .e<Service::cran>("https://cran.r-project.org/web/packages/.*?")
              .e<Service::ctan>("https://ctan.org/pkg/.*?")
              .e<Service::freedesktop_gitlab>(
                  "https://gitlab.freedesktop.org/.*?")
              .e<Service::gentoo>("https://gitweb.gentoo.org/.*?")
              .e<Service::github>("https://github.com/.*?")
              .e<Service::gitlab>("https://gitlab.com/.*?")
              .e<Service::gnome_gitlab>("https://gitlab.gnome.org/.*?")
              .e<Service::google_code>("https://code.google.com/archive/p/.*?")
              .e<Service::hackage>("https://hackage.haskell.org/package/.*?")
              .e<Service::heptapod>("https://foss.heptapod.net/.*?")
              .e<Service::kde_invent>("https://invent.kde.org/.*?")
              .e<Service::launchpad>("https://launchpad.net/.*?")
              .e<Service::osdn>("https://osdn.net/projects/.*?/")
              .e<Service::pear>("https://pear.php.net/package/.*?")
              .e<Service::pecl>("https://pecl.php.net/package/.*?")
              .e<Service::pypi>("https://pypi.org/project/.*?/")
              .e<Service::rubygems>("https://rubygems.org/gems/.*?")
              .e<Service::savannah>("https://savannah.gnu.org/projects/.*?")
              .e<Service::savannah_nongnu>(
                  "https://savannah.nongnu.org/projects/.*?")
              .e<Service::sourceforge>("https://sourceforge.net/projects/.*?")
              .e<Service::sourcehut>("https://sr.ht/.*?")
              .e<Service::vim>(
                  "https://www.vim.org/scripts/script.php?script_id=.*?")
              .build();

}  // namespace gyou

#endif  // INCLUDE_STRUCTS_SERVICE_REGEX_HPP_
