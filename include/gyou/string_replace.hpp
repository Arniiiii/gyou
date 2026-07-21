#ifndef INCLUDE_GYOU_STRING_REPLACE_HPP_
#define INCLUDE_GYOU_STRING_REPLACE_HPP_

// Source - https://stackoverflow.com/a/75954312
// Posted by Ignat Loskutov
// Retrieved 2026-07-21, License - CC BY-SA 4.0

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>

namespace gyou
{
    inline size_t CountOccurrences(std::string_view str,
                                   std::string_view needle)
    {
        size_t res = 0;
        size_t pos = 0;
        while ((pos = str.find(needle, pos)) != std::string_view::npos)
            {
                ++res;
                pos += needle.size();
            }
        return res;
    }

    inline std::string ReplaceNotLonger(std::string str, std::string_view what,
                                        std::string_view with)
    {
        assert(what.size() >= with.size());
        std::string_view::size_type wpos = 0;
        std::string_view::size_type rpos = 0;
        while (true)
            {
                auto new_rpos = str.find(what, rpos);
                if (new_rpos == std::string::npos)
                    {
                        new_rpos = str.size();
                    }
                auto n_diff = new_rpos - rpos;
                std::copy(str.begin() + static_cast<ptrdiff_t>(rpos),
                          str.begin() + static_cast<ptrdiff_t>(new_rpos),
                          str.begin() + static_cast<ptrdiff_t>(wpos));
                wpos += n_diff;
                rpos = new_rpos;
                if (rpos == str.size())
                    {
                        break;
                    }
                std::ranges::copy(with,
                                  str.begin() + static_cast<ptrdiff_t>(wpos));
                wpos += with.size();
                rpos += what.size();
            }
        str.resize(wpos);
        return str;
    }

    inline std::string ReplaceLonger(std::string str, std::string_view what,
                                     std::string_view with)
    {
        assert(what.size() < with.size());
        auto occurrences = CountOccurrences(str, what);
        auto rpos = str.size();
        auto wpos = rpos + (occurrences * (with.size() - what.size()));
        str.resize(wpos);

        while (wpos != rpos)
            {
                auto new_rpos = str.rfind(what, rpos - what.size());
                if (new_rpos == std::string::npos)
                    {
                        new_rpos = 0;
                    }
                else
                    {
                        new_rpos += what.size();
                    }
                auto n_diff = rpos - new_rpos;
                std::copy_backward(
                    str.begin() + static_cast<ptrdiff_t>(new_rpos),
                    str.begin() + static_cast<ptrdiff_t>(rpos),
                    str.begin() + static_cast<ptrdiff_t>(wpos));
                wpos -= n_diff;
                rpos = new_rpos;
                if (wpos == rpos)
                    {
                        break;
                    }
                std::ranges::copy_backward(
                    with, str.begin() + static_cast<ptrdiff_t>(wpos));
                wpos -= with.size();
                rpos -= what.size();
            }
        return str;
    }

    inline std::string Replace(std::string str, std::string_view what,
                               std::string_view with)
    {
        assert(!what.empty());
        if (what.size() >= with.size())
            {
                return ReplaceNotLonger(std::move(str), what, with);
            }
        return ReplaceLonger(std::move(str), what, with);
    }
}  // namespace gyou

#endif  // INCLUDE_GYOU_STRING_REPLACE_HPP_
