// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/core/Result.h>

using namespace ocf;

TEST(ResultTest, OkValue)
{
    auto res = Result<int, std::string>::Ok(42);
    EXPECT_TRUE(res.isOk());
    EXPECT_FALSE(res.isErr());
    EXPECT_EQ(res.unwrap(), 42);
}

TEST(ResultTest, ErrValue)
{
    auto res = Result<int, std::string>::Err("Error occurred");
    EXPECT_FALSE(res.isOk());
    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.unwrapErr(), "Error occurred");
}

TEST(ResultTest, BoolValue)
{
    auto res_ok = Result<int, std::string>::Ok(42);
    auto res_err = Result<int, std::string>::Err("Error occurred");
    EXPECT_TRUE(res_ok);
    EXPECT_FALSE(res_err);
}

TEST(ResultTest, UnwrapOr)
{
    auto res_ok = Result<int, std::string>::Ok(42);
    auto res_err = Result<int, std::string>::Err("Error occurred");
    EXPECT_EQ(res_ok.unwrapOr(100), 42);
    EXPECT_EQ(res_err.unwrapOr(100), 100);
}

TEST(ResultTest, VoidResult)
{
    using VoidResult = Result<void, std::string>;
    auto res_ok = VoidResult::Ok();
    EXPECT_TRUE(res_ok);
    EXPECT_FALSE(res_ok.isErr());
}
