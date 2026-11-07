// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/resource/SpriteSheet.h>
#include <ocf/resource/SpriteSheetLoader.h>

#include <fstream>
#include <iterator>
#include <string>

using namespace ocf;

namespace {

// The same sheet in both TexturePacker formats. "walk_02" is rotated and trimmed.
constexpr const char* SHEET_HASH = R"({
  "frames": {
    "walk_02.png": {
      "frame": { "x": 40, "y": 0, "w": 20, "h": 30 },
      "rotated": true, "trimmed": true,
      "spriteSourceSize": { "x": 4, "y": 2, "w": 20, "h": 30 },
      "sourceSize": { "w": 32, "h": 40 },
      "pivot": { "x": 0.5, "y": 1 }
    },
    "walk_01.png": {
      "frame": { "x": 0, "y": 0, "w": 32, "h": 40 },
      "rotated": false, "trimmed": false,
      "spriteSourceSize": { "x": 0, "y": 0, "w": 32, "h": 40 },
      "sourceSize": { "w": 32, "h": 40 }
    },
    "icon.png": { "frame": { "x": 0, "y": 40, "w": 16, "h": 16 } }
  },
  "meta": { "app": "https://www.codeandweb.com/texturepacker", "image": "../images/sheet.png",
            "format": "RGBA8888", "size": { "w": 128, "h": 64 }, "scale": "1" }
})";

constexpr const char* SHEET_ARRAY = R"({
  "frames": [
    { "filename": "walk_02.png",
      "frame": { "x": 40, "y": 0, "w": 20, "h": 30 },
      "rotated": true, "trimmed": true,
      "spriteSourceSize": { "x": 4, "y": 2, "w": 20, "h": 30 },
      "sourceSize": { "w": 32, "h": 40 },
      "pivot": { "x": 0.5, "y": 1 } },
    { "filename": "walk_01.png",
      "frame": { "x": 0, "y": 0, "w": 32, "h": 40 },
      "rotated": false, "trimmed": false,
      "spriteSourceSize": { "x": 0, "y": 0, "w": 32, "h": 40 },
      "sourceSize": { "w": 32, "h": 40 } },
    { "filename": "icon.png", "frame": { "x": 0, "y": 40, "w": 16, "h": 16 } }
  ],
  "meta": { "image": "../images/sheet.png", "size": { "w": 128, "h": 64 } }
})";

void expectVec2(const math::vec2& actual, float x, float y)
{
    EXPECT_FLOAT_EQ(actual.x, x);
    EXPECT_FLOAT_EQ(actual.y, y);
}

void expectSampleSheet(const SpriteSheet& sheet)
{
    EXPECT_EQ(sheet.getImagePath(), "sheets/images/sheet.png");
    EXPECT_EQ(sheet.getImageWidth(), 128u);
    EXPECT_EQ(sheet.getImageHeight(), 64u);
    ASSERT_EQ(sheet.getFrameCount(), 3u);

    const SpriteFrame* walk1 = sheet.getFrame("walk_01.png");
    ASSERT_NE(walk1, nullptr);
    EXPECT_FALSE(walk1->rotated);
    EXPECT_TRUE(walk1->textureRect.equals(math::Rect(0.0f, 0.0f, 32.0f, 40.0f)));
    expectVec2(walk1->offset, 0.0f, 0.0f);
    expectVec2(walk1->sourceSize, 32.0f, 40.0f);
    expectVec2(walk1->pivot, 0.5f, 0.5f);

    const SpriteFrame* walk2 = sheet.getFrame("walk_02.png");
    ASSERT_NE(walk2, nullptr);
    EXPECT_TRUE(walk2->rotated);
    EXPECT_TRUE(walk2->textureRect.equals(math::Rect(40.0f, 0.0f, 20.0f, 30.0f)));
    EXPECT_TRUE(walk2->getPackedRect().equals(math::Rect(40.0f, 0.0f, 30.0f, 20.0f)));
    expectVec2(walk2->offset, 4.0f, 2.0f);
    expectVec2(walk2->sourceSize, 32.0f, 40.0f);
    expectVec2(walk2->pivot, 0.5f, 1.0f);

    // Without sourceSize the frame is its own original image
    const SpriteFrame* icon = sheet.getFrame("icon.png");
    ASSERT_NE(icon, nullptr);
    expectVec2(icon->sourceSize, 16.0f, 16.0f);

    EXPECT_EQ(sheet.getFrame("missing.png"), nullptr);

    const std::vector<const SpriteFrame*> walk = sheet.getFramesWithPrefix("walk_");
    ASSERT_EQ(walk.size(), 2u);
    EXPECT_EQ(walk[0]->name, "walk_01.png");
    EXPECT_EQ(walk[1]->name, "walk_02.png");
}

std::string loadError(const std::string& json)
{
    Result<Ref<SpriteSheet>, std::string> result = SpriteSheetLoader::loadFromString(json);
    return result.isErr() ? result.unwrapErr() : std::string();
}

} // namespace

TEST(SpriteSheetTest, AddFrameReplacesSameName)
{
    SpriteSheet sheet;
    SpriteFrame frame = SpriteFrame::fromRect(math::Rect(0.0f, 0.0f, 8.0f, 8.0f));
    frame.name = "a";
    sheet.addFrame(frame);
    frame.textureRect = math::Rect(8.0f, 0.0f, 8.0f, 8.0f);
    sheet.addFrame(frame);

    ASSERT_EQ(sheet.getFrameCount(), 1u);
    EXPECT_FLOAT_EQ(sheet.getFrame("a")->textureRect.m_position.x, 8.0f);
}

TEST(SpriteSheetLoaderTest, ReadsHashFormat)
{
    auto result = SpriteSheetLoader::loadFromString(SHEET_HASH, SpriteSheetLoader::Format::Json,
                                                    "sheets/data");
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    expectSampleSheet(*result.unwrap().ptr());
}

TEST(SpriteSheetLoaderTest, ReadsArrayFormat)
{
    auto result = SpriteSheetLoader::loadFromString(SHEET_ARRAY, SpriteSheetLoader::Format::Auto,
                                                    "sheets/data");
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    expectSampleSheet(*result.unwrap().ptr());
}

TEST(SpriteSheetLoaderTest, ReportsErrors)
{
    EXPECT_NE(loadError("{").find("invalid JSON"), std::string::npos);
    EXPECT_EQ(loadError(R"({ "frames": {} })"), "meta: is required");
    EXPECT_EQ(loadError(R"({ "meta": { "image": "a.png" } })"), "frames: is required");
    EXPECT_EQ(loadError(R"({ "frames": 1, "meta": { "image": "a.png" } })"),
              "frames: must be an object or an array");
    EXPECT_EQ(loadError(R"({ "frames": { "a": { "frame": { "x": 0, "y": 0, "w": 4 } } },
                             "meta": { "image": "a.png" } })"),
              "frames/a/frame/h: is required");
    EXPECT_EQ(loadError(R"({ "frames": [ { "frame": { "x": 0, "y": 0, "w": 4, "h": 4 } } ],
                             "meta": { "image": "a.png" } })"),
              "frames/0/filename: must be a string");
    EXPECT_EQ(loadError(R"({ "frames": { "a": { "frame": { "x": 0, "y": 0, "w": 4, "h": 4 },
                             "rotated": 1 } }, "meta": { "image": "a.png" } })"),
              "frames/a/rotated: must be a boolean");
    EXPECT_EQ(loadError(R"({ "frames": { "a": { "frame": { "x": 0, "y": 0, "w": 0, "h": 4 } } },
                             "meta": { "image": "a.png" } })"),
              "frames/a: the region in the texture must have a non-negative position and a "
              "positive size");
    EXPECT_EQ(loadError(R"({ "frames": { "a": { "frame": { "x": 0, "y": 0, "w": 4, "h": 4 },
                             "trimmed": true, "spriteSourceSize": { "x": 2, "y": 0 },
                             "sourceSize": { "w": 4, "h": 4 } } },
                             "meta": { "image": "a.png" } })"),
              "frames/a: the trimmed image does not fit in sourceSize");
}

TEST(SpriteSheetLoaderTest, DetectsFormat)
{
    using Format = SpriteSheetLoader::Format;
    EXPECT_EQ(SpriteSheetLoader::detectFormat(SHEET_HASH), Format::Json);
    EXPECT_EQ(SpriteSheetLoader::detectFormat("\xEF\xBB\xBF \n {}"), Format::Json);
    EXPECT_EQ(SpriteSheetLoader::detectFormat(""), std::nullopt);
    EXPECT_EQ(SpriteSheetLoader::detectFormat("<TextureAtlas/>"), std::nullopt);
    EXPECT_EQ(loadError("<TextureAtlas/>"), "unknown sprite sheet format");
}

TEST(SpriteQuadTest, PlainRegion)
{
    const SpriteFrame frame = SpriteFrame::fromRect(math::Rect(16.0f, 0.0f, 16.0f, 32.0f));
    const math::vec2 textureSize(64.0f, 32.0f);

    SpriteQuad quad = computeSpriteQuad(frame, textureSize, false, false);
    expectVec2(quad.position, 0.0f, 0.0f);
    expectVec2(quad.size, 16.0f, 32.0f);
    expectVec2(quad.uvs[0], 0.25f, 0.0f); // top left
    expectVec2(quad.uvs[1], 0.25f, 1.0f); // bottom left
    expectVec2(quad.uvs[2], 0.5f, 1.0f);  // bottom right
    expectVec2(quad.uvs[3], 0.5f, 0.0f);  // top right

    quad = computeSpriteQuad(frame, textureSize, true, true);
    expectVec2(quad.uvs[0], 0.5f, 1.0f);
    expectVec2(quad.uvs[1], 0.5f, 0.0f);
    expectVec2(quad.uvs[2], 0.25f, 0.0f);
    expectVec2(quad.uvs[3], 0.25f, 1.0f);
}

TEST(SpriteQuadTest, RotatedFrame)
{
    // A 20x30 image stored rotated clockwise as 30x20 texels at (40, 0)
    SpriteFrame frame;
    frame.textureRect = math::Rect(40.0f, 0.0f, 20.0f, 30.0f);
    frame.rotated = true;
    frame.sourceSize = frame.textureRect.m_size;
    const math::vec2 textureSize(100.0f, 40.0f);

    const SpriteQuad quad = computeSpriteQuad(frame, textureSize, false, false);
    expectVec2(quad.size, 20.0f, 30.0f);
    // The top-left corner of the image is at the top-right corner of the packed region
    expectVec2(quad.uvs[0], 0.7f, 0.0f);
    expectVec2(quad.uvs[1], 0.4f, 0.0f);
    expectVec2(quad.uvs[2], 0.4f, 0.5f);
    expectVec2(quad.uvs[3], 0.7f, 0.5f);

    const SpriteQuad flipped = computeSpriteQuad(frame, textureSize, true, false);
    expectVec2(flipped.uvs[0], 0.7f, 0.5f);
    expectVec2(flipped.uvs[1], 0.4f, 0.5f);
    expectVec2(flipped.uvs[2], 0.4f, 0.0f);
    expectVec2(flipped.uvs[3], 0.7f, 0.0f);
}

TEST(SpriteQuadTest, TrimmedFrameMovesWhenFlipped)
{
    SpriteFrame frame;
    frame.textureRect = math::Rect(0.0f, 0.0f, 20.0f, 30.0f);
    frame.offset = math::vec2(4.0f, 2.0f);
    frame.sourceSize = math::vec2(32.0f, 40.0f);
    const math::vec2 textureSize(64.0f, 64.0f);

    expectVec2(computeSpriteQuad(frame, textureSize, false, false).position, 4.0f, 2.0f);
    expectVec2(computeSpriteQuad(frame, textureSize, true, false).position, 8.0f, 2.0f);
    expectVec2(computeSpriteQuad(frame, textureSize, false, true).position, 4.0f, 8.0f);
}

#ifdef OCF_TEST_ASSETS_DIR
TEST(SpriteSheetLoaderTest, ReadsSampleAsset)
{
    std::ifstream file(std::string(OCF_TEST_ASSETS_DIR) + "/spritesheets/sample.json",
                       std::ios::binary);
    ASSERT_TRUE(file);
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    auto result = SpriteSheetLoader::loadFromString(text, SpriteSheetLoader::Format::Auto,
                                                    "spritesheets");
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    const SpriteSheet& sheet = *result.unwrap().ptr();
    EXPECT_EQ(sheet.getImagePath(), "spritesheets/sample.png");
    EXPECT_GT(sheet.getFrameCount(), 0u);
}
#endif
