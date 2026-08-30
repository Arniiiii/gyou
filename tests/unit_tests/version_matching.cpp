#include <string>

#include <gtest/gtest.h>

#include <fmt/ostream.h>
#include <quill/core/LogLevel.h>

#include "gyou/consts/pcre2_regex_tag_ver.hpp"
#include "gyou/create_pcre2_regex.hpp"
#include "gyou/tag_to_portage_versions.hpp"
#include "quill_static.hpp"

TEST(Parsing, ParsingSomeTags)
{
    try
        {
            setup_quill("console", quill::LogLevel::TraceL3);
            auto tag_regex = gyou::create_pcre2_regex(
                std::string(gyou::PCRE2_REGEX_TAG_VERSION));

            EXPECT_EQ("1.2.3",
                      gyou::tag_to_portage_version(tag_regex, "1.2.3").value());
            EXPECT_EQ("1.92.0",
                      gyou::tag_to_portage_version(tag_regex, "boost-1.92.0")
                          .value());
            EXPECT_EQ(
                "8.8.0-rc",
                gyou::tag_to_portage_version(tag_regex, "8.8.0rc").value());
            EXPECT_EQ(
                "8.8.0-rc",
                gyou::tag_to_portage_version(tag_regex, "8.8.0_rc").value());
            EXPECT_EQ(
                "8.8.0-rc3",
                gyou::tag_to_portage_version(tag_regex, "8.8.0rc3").value());

            EXPECT_EQ("1.92.0-beta1", gyou::tag_to_portage_version(
                                          tag_regex, "boost-1.92.0.beta1")
                                          .value());
            EXPECT_EQ(
                "10655",
                gyou::tag_to_portage_version(tag_regex, "b10655").value());
        }
    catch (std::exception& exce)
        {
            fmt::println("Got exception: {}", exce.what());

            ADD_FAILURE();
        }
}
