// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>

#include <ocf/core/Reference.h>
#include <ocf/resource/TextLayout.h>
#include <ocf/resource/Font.h>

using namespace ocf;

namespace {

/**
 * Font with hand-written character definitions. Every visible glyph is 8x10 texels,
 * advances by 10 and sits 2px below the top of the line.
 */
class FakeFont : public Font {
public:
    FakeFont()
    {
        setLineHeight(16.0f);

        addGlyph(U'A', 0.0f, 0, 10.0f);
        addGlyph(U'B', 8.0f, 0, 10.0f);
        addGlyph(U'C', 16.0f, 1, 10.0f);
        addGlyph(U'あ', 24.0f, 0, 16.0f); // あ: wider advance

        // Space has no bitmap, only an advance
        addCharacterDefinition(U' ', {.x = 0.0f, .y = 0.0f, .width = 0.0f, .height = 0.0f,
                                      .xoffset = 0.0f, .yoffset = 0.0f, .xadvance = 5.0f,
                                      .page = 0});
    }

    FontAtlas* createFontAtlas(Engine&) override { return nullptr; }

private:
    void addGlyph(char32_t c, float atlasX, int page, float advance)
    {
        addCharacterDefinition(c, {.x = atlasX, .y = 4.0f, .width = 8.0f, .height = 10.0f,
                                   .xoffset = 1.0f, .yoffset = 2.0f, .xadvance = advance,
                                   .page = page});
    }
};

class TextLayoutTest : public ::testing::Test {
protected:
    Ref<FakeFont> m_font = Ref<FakeFont>(new FakeFont());
};

} // namespace

TEST_F(TextLayoutTest, EmptyText)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"");

    EXPECT_TRUE(result.quads.empty());
    EXPECT_EQ(result.lineCount, 0);
    EXPECT_FLOAT_EQ(result.size.x, 0.0f);
    EXPECT_FLOAT_EQ(result.size.y, 0.0f);
}

TEST_F(TextLayoutTest, SingleLine)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"AB");

    ASSERT_EQ(result.quads.size(), 2u);
    EXPECT_EQ(result.lineCount, 1);
    EXPECT_FLOAT_EQ(result.size.x, 20.0f);
    EXPECT_FLOAT_EQ(result.size.y, 16.0f);

    // Position = pen + offset
    EXPECT_FLOAT_EQ(result.quads[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(result.quads[0].position.y, 2.0f);
    EXPECT_FLOAT_EQ(result.quads[1].position.x, 11.0f);
    EXPECT_FLOAT_EQ(result.quads[1].position.y, 2.0f);

    EXPECT_FLOAT_EQ(result.quads[0].size.x, 8.0f);
    EXPECT_FLOAT_EQ(result.quads[0].size.y, 10.0f);
}

TEST_F(TextLayoutTest, AtlasRectAndPage)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"BC");

    ASSERT_EQ(result.quads.size(), 2u);
    EXPECT_FLOAT_EQ(result.quads[0].texRect.m_position.x, 8.0f);
    EXPECT_FLOAT_EQ(result.quads[0].texRect.m_position.y, 4.0f);
    EXPECT_FLOAT_EQ(result.quads[0].texRect.m_size.x, 8.0f);
    EXPECT_FLOAT_EQ(result.quads[0].texRect.m_size.y, 10.0f);
    EXPECT_EQ(result.quads[0].page, 0);
    EXPECT_EQ(result.quads[1].page, 1);
}

TEST_F(TextLayoutTest, SpaceAdvancesWithoutQuad)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"A B");

    ASSERT_EQ(result.quads.size(), 2u);
    EXPECT_FLOAT_EQ(result.quads[1].position.x, 10.0f + 5.0f + 1.0f);
    EXPECT_FLOAT_EQ(result.size.x, 25.0f);
}

TEST_F(TextLayoutTest, MissingCharacterIsSkipped)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"AZB");

    ASSERT_EQ(result.quads.size(), 2u);
    EXPECT_FLOAT_EQ(result.quads[1].position.x, 11.0f);
    EXPECT_FLOAT_EQ(result.size.x, 20.0f);
}

TEST_F(TextLayoutTest, NonAsciiCharacter)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"あA");

    ASSERT_EQ(result.quads.size(), 2u);
    EXPECT_FLOAT_EQ(result.quads[1].position.x, 16.0f + 1.0f);
    EXPECT_FLOAT_EQ(result.size.x, 26.0f);
}

TEST_F(TextLayoutTest, MultiLine)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"AB\nA");

    ASSERT_EQ(result.quads.size(), 3u);
    EXPECT_EQ(result.lineCount, 2);
    EXPECT_FLOAT_EQ(result.size.x, 20.0f);
    EXPECT_FLOAT_EQ(result.size.y, 32.0f);

    EXPECT_FLOAT_EQ(result.quads[2].position.x, 1.0f);
    EXPECT_FLOAT_EQ(result.quads[2].position.y, 16.0f + 2.0f);
}

TEST_F(TextLayoutTest, CarriageReturnIsIgnored)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"A\r\nB");

    ASSERT_EQ(result.quads.size(), 2u);
    EXPECT_EQ(result.lineCount, 2);
    EXPECT_FLOAT_EQ(result.quads[1].position.x, 1.0f);
    EXPECT_FLOAT_EQ(result.size.x, 10.0f);
}

TEST_F(TextLayoutTest, TrailingNewlineAddsEmptyLine)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"A\n");

    EXPECT_EQ(result.lineCount, 2);
    EXPECT_FLOAT_EQ(result.size.y, 32.0f);
}

TEST_F(TextLayoutTest, AlignCenter)
{
    // Line widths: 30 and 10 -> second line is shifted by (30 - 10) / 2
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"AAA\nA", TextAlignment::Center);

    ASSERT_EQ(result.quads.size(), 4u);
    EXPECT_FLOAT_EQ(result.quads[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(result.quads[3].position.x, 10.0f + 1.0f);
}

TEST_F(TextLayoutTest, AlignRight)
{
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"AAA\nA", TextAlignment::Right);

    ASSERT_EQ(result.quads.size(), 4u);
    EXPECT_FLOAT_EQ(result.quads[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(result.quads[3].position.x, 20.0f + 1.0f);
}

TEST_F(TextLayoutTest, AlignRightWithEmptyLine)
{
    // The empty middle line must not steal quads from the following line
    TextLayoutResult result = TextLayout::layout(*m_font.ptr(), U"AA\n\nA", TextAlignment::Right);

    ASSERT_EQ(result.quads.size(), 3u);
    EXPECT_EQ(result.lineCount, 3);
    EXPECT_FLOAT_EQ(result.quads[2].position.x, 10.0f + 1.0f);
    EXPECT_FLOAT_EQ(result.quads[2].position.y, 32.0f + 2.0f);
}
