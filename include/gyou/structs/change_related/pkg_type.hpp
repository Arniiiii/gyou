#ifndef INCLUDE_STRUCTS_PKG_TYPE_HPP_
#define INCLUDE_STRUCTS_PKG_TYPE_HPP_

namespace gyou
{

    enum class PackageType : std::uint8_t
    {
        Unknown,
        ReleaseOrTag,
        Commit,
    };
}

#endif  // INCLUDE_STRUCTS_PKG_TYPE_HPP_
