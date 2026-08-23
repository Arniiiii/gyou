#ifndef INCLUDE_UTILS_BOOST_PROCESS_FMT_HPP_
#define INCLUDE_UTILS_BOOST_PROCESS_FMT_HPP_

#include <boost/process/environment.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <quill/DeferredFormatCodec.h>
#include <quill/bundled/fmt/ranges.h>
#include <quill/bundled/fmt/std.h>

template <>
struct fmt::formatter<boost::process::environment::key_value_pair, char>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <class FmtContext> FmtContext::iterator format(
        const boost::process::environment::key_value_pair& key_value_pair,
        FmtContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}\n", key_value_pair.string());
    }
};
static_assert(
    fmt::formattable<boost::process::environment::key_value_pair, char>);

template <> struct fmt::formatter<boost::process::environment::key, char>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <class FmtContext>
    FmtContext::iterator format(const boost::process::environment::key& key,
                                FmtContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}\n", key.string());
    }
};
static_assert(fmt::formattable<boost::process::environment::key, char>);

template <> struct fmt::formatter<boost::process::environment::value, char>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <class FmtContext>
    FmtContext::iterator format(const boost::process::environment::value& val,
                                FmtContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", val.string());
    }
};

static_assert(fmt::formattable<boost::process::environment::value, char>);

template <> struct fmtquill::formatter<boost::process::environment::key, char>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <class FmtContext>
    FmtContext::iterator format(const boost::process::environment::key& key,
                                FmtContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", key.string());
    }
};
static_assert(fmtquill::formattable<boost::process::environment::key, char>);

template <> struct fmtquill::formatter<boost::process::environment::value, char>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <class FmtContext>
    FmtContext::iterator format(const boost::process::environment::value& val,
                                FmtContext& ctx) const
    {
        return fmtquill::format_to(ctx.out(), "{}", val.string());
    }
};
static_assert(fmtquill::formattable<boost::process::environment::value, char>);

template <>
struct fmtquill::formatter<boost::process::environment::key_value_pair, char>
{
    template <class ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <class FmtContext> FmtContext::iterator format(
        const boost::process::environment::key_value_pair& val,
        FmtContext& ctx) const
    {
        return fmtquill::format_to(ctx.out(), "{}", val.string());
    }
};

static_assert(
    fmtquill::formattable<boost::process::environment::key_value_pair, char>);

template <> struct quill::Codec<boost::process::environment::key_value_pair>
    : quill::DeferredFormatCodec<boost::process::environment::key_value_pair>
{
};

template <> struct quill::Codec<boost::process::environment::key>
    : quill::DeferredFormatCodec<boost::process::environment::key>
{
};

template <> struct quill::Codec<boost::process::environment::value>
    : quill::DeferredFormatCodec<boost::process::environment::value>
{
};

#endif  // INCLUDE_UTILS_BOOST_PROCESS_FMT_HPP_
