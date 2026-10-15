// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/core/base64.h>

#include <string_view>
#include <vector>

using namespace ocf;

namespace {

std::vector<uint8_t> bytes(std::string_view text)
{
    return std::vector<uint8_t>(text.begin(), text.end());
}

} // namespace

TEST(Base64Test, EncodesTheTestVectorsOfRfc4648)
{
    EXPECT_EQ(encodeBase64(bytes("")), "");
    EXPECT_EQ(encodeBase64(bytes("f")), "Zg==");
    EXPECT_EQ(encodeBase64(bytes("fo")), "Zm8=");
    EXPECT_EQ(encodeBase64(bytes("foo")), "Zm9v");
    EXPECT_EQ(encodeBase64(bytes("foob")), "Zm9vYg==");
    EXPECT_EQ(encodeBase64(bytes("fooba")), "Zm9vYmE=");
    EXPECT_EQ(encodeBase64(bytes("foobar")), "Zm9vYmFy");
}

TEST(Base64Test, DecodesTheTestVectorsOfRfc4648)
{
    EXPECT_EQ(decodeBase64(""), bytes(""));
    EXPECT_EQ(decodeBase64("Zg=="), bytes("f"));
    EXPECT_EQ(decodeBase64("Zm8="), bytes("fo"));
    EXPECT_EQ(decodeBase64("Zm9v"), bytes("foo"));
    EXPECT_EQ(decodeBase64("Zm9vYg=="), bytes("foob"));
    EXPECT_EQ(decodeBase64("Zm9vYmE="), bytes("fooba"));
    EXPECT_EQ(decodeBase64("Zm9vYmFy"), bytes("foobar"));
}

TEST(Base64Test, RoundTripsEveryByte)
{
    std::vector<uint8_t> data;
    for (int i = 0; i < 256; i++) {
        data.push_back(static_cast<uint8_t>(i));
    }
    for (size_t size = 0; size <= data.size(); size += 37) {
        const std::vector<uint8_t> part(data.begin(), data.begin() + size);
        EXPECT_EQ(decodeBase64(encodeBase64(part)), part) << size;
    }
    EXPECT_EQ(encodeBase64(data).find_first_not_of(
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="),
              std::string::npos);
}

TEST(Base64Test, RejectsInvalidText)
{
    EXPECT_FALSE(decodeBase64("Zg="));      // Length
    EXPECT_FALSE(decodeBase64("Zm9vY"));    // Length
    EXPECT_FALSE(decodeBase64("Z==="));     // Padding too early in the group
    EXPECT_FALSE(decodeBase64("Zg==Zm9v")); // Padding before the end
    EXPECT_FALSE(decodeBase64("Zm=v"));     // Data after padding
    EXPECT_FALSE(decodeBase64("Zm9*"));     // Character
    EXPECT_FALSE(decodeBase64("Zm9v\n"));   // Whitespace
}
