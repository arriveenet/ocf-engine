// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/audio/tracker/SongNotation.h>

using namespace ocf::audio::tracker;

TEST(TrackerNotationTest, NoteToString)
{
    EXPECT_EQ(noteToString(ChannelType::Pulse1, 1), "C-0");
    EXPECT_EQ(noteToString(ChannelType::Pulse1, 2), "C#0");
    EXPECT_EQ(noteToString(ChannelType::Pulse1, 58), "A-4");
    EXPECT_EQ(noteToString(ChannelType::Triangle, NOTE_MAX), "B-7");
    EXPECT_EQ(noteToString(ChannelType::Pulse1, NOTE_NONE), "...");
    EXPECT_EQ(noteToString(ChannelType::Pulse1, NOTE_OFF), "---");
    EXPECT_EQ(noteToString(ChannelType::Pulse1, NOTE_RELEASE), "===");
    EXPECT_EQ(noteToString(ChannelType::Noise, 1), "0-#");
    EXPECT_EQ(noteToString(ChannelType::Noise, NOISE_NOTE_MAX), "F-#");
    EXPECT_EQ(noteToString(ChannelType::Noise, NOTE_OFF), "---");
}

TEST(TrackerNotationTest, NoteRoundTrip)
{
    for (int note = NOTE_MIN; note <= NOTE_MAX; note++) {
        const std::string text = noteToString(ChannelType::Pulse2, static_cast<uint8_t>(note));
        EXPECT_EQ(noteFromString(ChannelType::Pulse2, text), note) << text;
    }
    for (int note = NOTE_MIN; note <= NOISE_NOTE_MAX; note++) {
        const std::string text = noteToString(ChannelType::Noise, static_cast<uint8_t>(note));
        EXPECT_EQ(noteFromString(ChannelType::Noise, text), note) << text;
    }
    for (uint8_t note : {NOTE_NONE, NOTE_OFF, NOTE_RELEASE}) {
        EXPECT_EQ(noteFromString(ChannelType::Dpcm, noteToString(ChannelType::Dpcm, note)), note);
    }
}

TEST(TrackerNotationTest, NoteFromStringRejectsInvalidText)
{
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, ""));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "C4"));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "C-8"));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "E#4"));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "H-4"));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "c-4"));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "C-44"));
    EXPECT_FALSE(noteFromString(ChannelType::Pulse1, "0-#"));
    EXPECT_FALSE(noteFromString(ChannelType::Noise, "C-4"));
    EXPECT_FALSE(noteFromString(ChannelType::Noise, "G-#"));
}

TEST(TrackerNotationTest, NoteFromStringAcceptsLowercaseNoiseHex)
{
    EXPECT_EQ(noteFromString(ChannelType::Noise, "a-#"), 11);
}

TEST(TrackerNotationTest, EffectRoundTrip)
{
    EXPECT_EQ(effectToString(Effect{EffectType::Speed, 0x06}), "F06");
    EXPECT_EQ(effectToString(Effect{EffectType::Jump, 0xAB}), "BAB");
    EXPECT_EQ(effectToString(Effect{EffectType::Skip, 0x10}), "D10");
    EXPECT_EQ(effectToString(Effect{EffectType::Halt, 0x00}), "C00");
    EXPECT_EQ(effectToString(Effect{}), "...");

    EXPECT_EQ(effectFromString("F06"), (Effect{EffectType::Speed, 0x06}));
    EXPECT_EQ(effectFromString("Bab"), (Effect{EffectType::Jump, 0xAB}));
    EXPECT_EQ(effectFromString("..."), Effect{});
}

TEST(TrackerNotationTest, EffectFromStringRejectsInvalidText)
{
    EXPECT_FALSE(effectFromString(""));
    EXPECT_FALSE(effectFromString("F6"));
    EXPECT_FALSE(effectFromString("F006"));
    EXPECT_FALSE(effectFromString("f06"));
    EXPECT_FALSE(effectFromString("Z06"));
    EXPECT_FALSE(effectFromString("F0G"));
}

TEST(TrackerNotationTest, ChannelAndSequenceNames)
{
    for (size_t i = 0; i < CHANNEL_COUNT; i++) {
        const auto channel = static_cast<ChannelType>(i);
        EXPECT_EQ(channelTypeFromString(channelTypeToString(channel)), channel);
    }
    EXPECT_EQ(channelTypeToString(ChannelType::Triangle), "triangle");
    EXPECT_FALSE(channelTypeFromString("square"));

    for (size_t i = 0; i < SEQUENCE_TYPE_COUNT; i++) {
        const auto type = static_cast<SequenceType>(i);
        EXPECT_EQ(sequenceTypeFromString(sequenceTypeToString(type)), type);
    }
    EXPECT_EQ(sequenceTypeToString(SequenceType::Duty), "duty");
    EXPECT_FALSE(sequenceTypeFromString("arpeggio"));
}

TEST(TrackerNotationTest, SequenceToMml)
{
    Sequence sequence;
    EXPECT_EQ(sequenceToMml(sequence), "");

    sequence.values = {15, 12, 10, 8, 4, 0};
    EXPECT_EQ(sequenceToMml(sequence), "15 12 10 8 4 0");

    sequence.loopPoint = 2;
    sequence.releasePoint = 4;
    EXPECT_EQ(sequenceToMml(sequence), "15 12 | 10 8 / 4 0");

    sequence.values = {-3, 3};
    sequence.loopPoint = 0;
    sequence.releasePoint = 0;
    EXPECT_EQ(sequenceToMml(sequence), "| / -3 3");
}

TEST(TrackerNotationTest, SequenceFromMml)
{
    const std::optional<Sequence> sequence = sequenceFromMml("15 12 | 10 8 / 4 0");
    ASSERT_TRUE(sequence);
    EXPECT_EQ(sequence->values, (std::vector<int8_t>{15, 12, 10, 8, 4, 0}));
    EXPECT_EQ(sequence->loopPoint, 2);
    EXPECT_EQ(sequence->releasePoint, 4);

    // Markers do not need spaces around them
    const std::optional<Sequence> compact = sequenceFromMml("1|-2/3");
    ASSERT_TRUE(compact);
    EXPECT_EQ(compact->values, (std::vector<int8_t>{1, -2, 3}));
    EXPECT_EQ(compact->loopPoint, 1);
    EXPECT_EQ(compact->releasePoint, 2);

    const std::optional<Sequence> empty = sequenceFromMml("  ");
    ASSERT_TRUE(empty);
    EXPECT_TRUE(empty->isEmpty());
}

TEST(TrackerNotationTest, SequenceMmlRoundTrip)
{
    Sequence sequence;
    sequence.values = {0, 1, 2, 3, -1};
    sequence.loopPoint = 1;
    sequence.releasePoint = 3;
    EXPECT_EQ(sequenceFromMml(sequenceToMml(sequence)), sequence);
}

TEST(TrackerNotationTest, SequenceFromMmlRejectsInvalidText)
{
    EXPECT_FALSE(sequenceFromMml("15 x 3"));
    EXPECT_FALSE(sequenceFromMml("128"));
    EXPECT_FALSE(sequenceFromMml("-129"));
    EXPECT_FALSE(sequenceFromMml("1 | 2 | 3"));
    EXPECT_FALSE(sequenceFromMml("1 / 2 / 3"));
    EXPECT_FALSE(sequenceFromMml("1 2 |"));
    EXPECT_FALSE(sequenceFromMml("1 2 /"));
}
