// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/renderer/TextureSampler.h>

using namespace ocf;

TEST(TextureSamplerTest, ConstructorInitializesComponents)
{
    TextureSampler sampler(TextureSampler::MinFilter::Linear, TextureSampler::MagFilter::Linear);

    EXPECT_EQ(sampler.getMinFilter(), TextureSampler::MinFilter::Linear);
    EXPECT_EQ(sampler.getMagFilter(), TextureSampler::MagFilter::Linear);
    EXPECT_EQ(sampler.getWrapModeR(), TextureSampler::WrapMode::ClampToEdge);
    EXPECT_EQ(sampler.getWrapModeS(), TextureSampler::WrapMode::ClampToEdge);
    EXPECT_EQ(sampler.getWrapModeT(), TextureSampler::WrapMode::ClampToEdge);
}

TEST(TextureSamplerTest, Anisotropy)
{
    TextureSampler sampler;
    sampler.setAnisotropy(16.0f);
    EXPECT_FLOAT_EQ(sampler.getAnisotropy(), 16.0f);

    sampler.setAnisotropy(1.0f);
    EXPECT_FLOAT_EQ(sampler.getAnisotropy(), 1.0f);

    sampler.setAnisotropy(1.5f);
    EXPECT_FLOAT_EQ(sampler.getAnisotropy(), 1.0f);

    sampler.setAnisotropy(0.0f);
    EXPECT_FLOAT_EQ(sampler.getAnisotropy(), 1.0f);

    sampler.setAnisotropy(3.0f);
    EXPECT_FLOAT_EQ(sampler.getAnisotropy(), 2.0f);

    sampler.setAnisotropy(32.0f);
    EXPECT_FLOAT_EQ(sampler.getAnisotropy(), 16.0f);
}