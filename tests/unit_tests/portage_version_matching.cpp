#include <string>

#include <gtest/gtest.h>

#include <fmt/ostream.h>
#include <quill/core/LogLevel.h>

#include "gyou/consts/pcre2_regex_portage_ver.hpp"
#include "gyou/create_pcre2_regex.hpp"
#include "gyou/parse_portage_ver.hpp"
#include "gyou/structs/portage_ver.hpp"
#include "quill_static.hpp"

TEST(Parsing, ParsingPortageVersions)
{
    try
        {
            setup_quill("console", quill::LogLevel::TraceL3);
            auto tag_regex = gyou::create_pcre2_regex(
                std::string(gyou::PCRE2_REGEX_PORTAGE_VERSION));

            {
                const auto expected = gyou::ParsedEbuildStem{
                    .pn = "sth", .pn_inval = "", .ver = "1.2.3", .rev = ""};
                const auto parsed
                    = gyou::parse_ebuild_name(tag_regex, "sth-1.2.3").value();
                EXPECT_EQ(expected.pn, parsed.pn);
                EXPECT_EQ(expected.pn_inval, parsed.pn_inval);
                EXPECT_EQ(expected.ver, parsed.ver);
                EXPECT_EQ(expected.rev, parsed.rev);
            }

            {
                const auto expected
                    = gyou::ParsedEbuildStem{.pn = "sth",
                                             .pn_inval = "",
                                             .ver = "1.2.3_beta1",
                                             .rev = ""};
                const auto parsed
                    = gyou::parse_ebuild_name(tag_regex, "sth-1.2.3_beta1")
                          .value();
                EXPECT_EQ(expected.pn, parsed.pn);
                EXPECT_EQ(expected.pn_inval, parsed.pn_inval);
                EXPECT_EQ(expected.ver, parsed.ver);
                EXPECT_EQ(expected.rev, parsed.rev);
            }

            {
                const auto expected
                    = gyou::ParsedEbuildStem{.pn = "sth",
                                             .pn_inval = "",
                                             .ver = "1.2.3_beta1",
                                             .rev = "25"};
                const auto parsed
                    = gyou::parse_ebuild_name(tag_regex, "sth-1.2.3_beta1-r25")
                          .value();
                EXPECT_EQ(expected.pn, parsed.pn);
                EXPECT_EQ(expected.pn_inval, parsed.pn_inval);
                EXPECT_EQ(expected.ver, parsed.ver);
                EXPECT_EQ(expected.rev, parsed.rev);
            }

            {
                const auto expected
                    = gyou::ParsedEbuildStem{.pn = "sth-1.0",
                                             .pn_inval = "-1.0",
                                             .ver = "1.2.3_beta1",
                                             .rev = "25"};
                const auto parsed = gyou::parse_ebuild_name(
                                        tag_regex, "sth-1.0-1.2.3_beta1-r25")
                                        .value();
                EXPECT_EQ(expected.pn, parsed.pn);
                EXPECT_EQ(expected.pn_inval, parsed.pn_inval);
                EXPECT_EQ(expected.ver, parsed.ver);
                EXPECT_EQ(expected.rev, parsed.rev);
            }
        }
    catch (std::exception& exce)
        {
            fmt::println("Got exception: {}", exce.what());

            ADD_FAILURE();
        }
}
