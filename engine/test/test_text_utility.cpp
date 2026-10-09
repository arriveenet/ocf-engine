// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/core/TextUtility.h>

using namespace ocf;

TEST(TextUtilityTest, Empty)
{
    EXPECT_TRUE(TextUtility::utf8ToUtf32("").empty());
}

TEST(TextUtilityTest, Ascii)
{
    EXPECT_EQ(TextUtility::utf8ToUtf32("Hello, World!"), U"Hello, World!");
}

TEST(TextUtilityTest, MultiByte)
{
    // 2-byte (é), 3-byte (あ, 漢) and 4-byte (😀) sequences
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xC3\xA9"), U"é");
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xE3\x81\x82\xE6\xBC\xA2"), U"あ漢");
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xF0\x9F\x98\x80"), U"\U0001F600");
}

TEST(TextUtilityTest, Mixed)
{
    EXPECT_EQ(TextUtility::utf8ToUtf32("a\xE3\x81\x82" "b\n"), U"aあb\n");
}

TEST(TextUtilityTest, StrayContinuationByte)
{
    EXPECT_EQ(TextUtility::utf8ToUtf32("a\x80" "b"), U"a�b");
}

TEST(TextUtilityTest, InvalidLeadByte)
{
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xFF" "a"), U"�a");
}

TEST(TextUtilityTest, TruncatedSequenceKeepsFollowingCharacter)
{
    // 3-byte lead followed by only one continuation byte, then ASCII
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xE3\x81" "a"), U"�a");
    // Truncated at end of input
    EXPECT_EQ(TextUtility::utf8ToUtf32("a\xE3\x81"), U"a�");
}

TEST(TextUtilityTest, OverlongEncoding)
{
    // '/' encoded with two bytes
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xC0\xAF"), U"�");
}

TEST(TextUtilityTest, Surrogate)
{
    // U+D800 encoded directly
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xED\xA0\x80"), U"�");
}

TEST(TextUtilityTest, AboveMaxCodePoint)
{
    // U+110000
    EXPECT_EQ(TextUtility::utf8ToUtf32("\xF4\x90\x80\x80"), U"�");
}
