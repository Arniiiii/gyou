#ifndef INCLUDE_GYOU_CREATE_PCRE2_REGEX_HPP_
#define INCLUDE_GYOU_CREATE_PCRE2_REGEX_HPP_

#include <string>

#include <reflex/pcre2matcher.h>

namespace gyou
{
    reflex::PCRE2UTFMatcher create_pcre2_regex(std::string const& regex_str);

}

#endif  // INCLUDE_GYOU_CREATE_PCRE2_REGEX_HPP_
