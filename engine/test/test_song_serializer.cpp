// SPDX-License-Identifier: MIT
#include <gtest/gtest.h>
#include <ocf/audio/tracker/SongNotation.h>
#include <ocf/audio/tracker/SongSerializer.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using namespace ocf::audio::tracker;

namespace {

// The complete example of docs/tracker-format.md. Keep the two identical.
constexpr const char* EXAMPLE_JSON = R"json({
  "format": "ocf-tracker-song",
  "version": 1,
  "title": "Example",
  "author": "ocf",
  "speed": 6,
  "tempo": 150,
  "rowsPerPattern": 16,
  "channels": [
    { "type": "pulse1", "effectColumns": 1 },
    { "type": "pulse2", "effectColumns": 1 },
    { "type": "triangle", "effectColumns": 1 },
    { "type": "noise", "effectColumns": 1 },
    { "type": "dpcm", "effectColumns": 1 }
  ],
  "instruments": [
    {
      "index": 0,
      "name": "Lead",
      "sequences": {
        "volume": { "values": [15, 13, 11, 10, 9, 8], "loop": -1, "release": -1 },
        "duty": { "values": [2], "loop": -1, "release": -1 }
      }
    },
    {
      "index": 1,
      "name": "Bass",
      "sequences": {
        "volume": { "values": [15], "loop": -1, "release": -1 }
      }
    }
  ],
  "frames": [
    [0, 0, 0, 0, 0],
    [1, 0, 0, 0, 0]
  ],
  "patterns": [
    {
      "channel": 0,
      "index": 0,
      "rows": [
        { "row": 0, "note": "C-4", "instrument": 0 },
        { "row": 4, "note": "E-4" },
        { "row": 8, "note": "G-4" },
        { "row": 12, "note": "---" }
      ]
    },
    {
      "channel": 0,
      "index": 1,
      "rows": [
        { "row": 0, "note": "B-3", "instrument": 0, "volume": 12 },
        { "row": 8, "note": "D-4" },
        { "row": 15, "effects": ["B00"] }
      ]
    },
    {
      "channel": 2,
      "index": 0,
      "rows": [
        { "row": 0, "note": "C-3", "instrument": 1 },
        { "row": 8, "note": "G-2" }
      ]
    }
  ]
}
)json";

uint8_t note(ChannelType channel, const char* text)
{
    return noteFromString(channel, text).value();
}

Sequence sequence(const char* mml)
{
    return sequenceFromMml(mml).value();
}

Song createExampleSong()
{
    Song song = Song::createDefault();
    song.title = "Example";
    song.author = "ocf";
    song.setRowsPerPattern(16);
    song.frames = {Frame{0, 0, 0, 0, 0}, Frame{1, 0, 0, 0, 0}};

    song.instruments.resize(2);
    song.instruments[0].name = "Lead";
    song.instruments[0].getSequence(SequenceType::Volume) = sequence("15 13 11 10 9 8");
    song.instruments[0].getSequence(SequenceType::Duty) = sequence("2");
    song.instruments[1].name = "Bass";
    song.instruments[1].getSequence(SequenceType::Volume) = sequence("15");

    const ChannelType pulse = ChannelType::Pulse1;
    Pattern& first = song.getPattern(pulse, 0);
    first.rows[0].note = note(pulse, "C-4");
    first.rows[0].instrument = 0;
    first.rows[4].note = note(pulse, "E-4");
    first.rows[8].note = note(pulse, "G-4");
    first.rows[12].note = NOTE_OFF;

    Pattern& second = song.getPattern(pulse, 1);
    second.rows[0].note = note(pulse, "B-3");
    second.rows[0].instrument = 0;
    second.rows[0].volume = 12;
    second.rows[8].note = note(pulse, "D-4");
    second.rows[15].effects[0] = Effect{EffectType::Jump, 0};

    const ChannelType triangle = ChannelType::Triangle;
    Pattern& bass = song.getPattern(triangle, 0);
    bass.rows[0].note = note(triangle, "C-3");
    bass.rows[0].instrument = 1;
    bass.rows[8].note = note(triangle, "G-2");
    return song;
}

/** A song that uses every field. */
Song createFullSong()
{
    Song song = Song::createDefault();
    song.title = "Title with \"quotes\", \\ and 日本語";
    song.author = "Author\nwith a newline";
    song.speed = 3;
    song.tempo = 200;
    song.setRowsPerPattern(32);
    song.effectColumns = {1, 2, 3, 4, 2};
    song.frames = {Frame{0, 1, 2, 3, 4}, Frame{5, 5, 5, 5, 5}, Frame{255, 0, 0, 0, 0}};

    song.instruments.resize(3);
    song.instruments[0].name = "Pluck";
    song.instruments[0].getSequence(SequenceType::Volume) = sequence("15 12 | 10 8 / 4 0");
    song.instruments[0].getSequence(SequenceType::Duty) = sequence("| 0 1 2 3");
    song.instruments[2].name = "No sequences";

    Pattern& pulse = song.getPattern(ChannelType::Pulse2, 1);
    pulse.rows[0] = Cell{note(ChannelType::Pulse2, "C#7"), 2, 15, {}};
    pulse.rows[1].note = NOTE_RELEASE;
    pulse.rows[2].effects[1] = Effect{EffectType::Speed, 0x96};
    pulse.rows[31].volume = 0;

    Pattern& noise = song.getPattern(ChannelType::Noise, 3);
    noise.rows[5].note = note(ChannelType::Noise, "F-#");
    noise.rows[5].effects = {Effect{EffectType::Halt, 0}, Effect{}, Effect{},
                             Effect{EffectType::Skip, 0x1F}};

    Pattern& dpcm = song.getPattern(ChannelType::Dpcm, 4);
    dpcm.rows[10].note = note(ChannelType::Dpcm, "A-0");

    song.dpcmSamples.resize(3);
    song.dpcmSamples[0].name = "Kick";
    for (uint8_t i = 0; i < 33; i++) {
        song.dpcmSamples[0].data.push_back(static_cast<uint8_t>(i * 37));
    }
    song.dpcmSamples[0].rate = 13;
    song.dpcmSamples[0].restLevel = 0;
    song.dpcmSamples[2].name = "One byte";
    song.dpcmSamples[2].data = {0xFF};
    *song.instruments[0].findDpcmKey(note(ChannelType::Dpcm, "C-3")) = DpcmKey{0, 13, false};
    *song.instruments[0].findDpcmKey(note(ChannelType::Dpcm, "B-7")) = DpcmKey{2, 0, true};
    *song.instruments[2].findDpcmKey(NOTE_MIN) = DpcmKey{0, 15, false};
    return song;
}

/** The written format leaves out empty patterns, so they are compared as missing. */
void expectSameSong(const Song& expected, const Song& actual)
{
    EXPECT_EQ(actual.title, expected.title);
    EXPECT_EQ(actual.author, expected.author);
    EXPECT_EQ(actual.speed, expected.speed);
    EXPECT_EQ(actual.tempo, expected.tempo);
    EXPECT_EQ(actual.rowsPerPattern, expected.rowsPerPattern);
    EXPECT_EQ(actual.effectColumns, expected.effectColumns);
    EXPECT_EQ(actual.frames, expected.frames);

    ASSERT_EQ(actual.instruments.size(), expected.instruments.size());
    for (size_t i = 0; i < expected.instruments.size(); i++) {
        EXPECT_EQ(actual.instruments[i].name, expected.instruments[i].name) << i;
        EXPECT_EQ(actual.instruments[i].sequences, expected.instruments[i].sequences) << i;
        EXPECT_EQ(actual.instruments[i].dpcmKeys, expected.instruments[i].dpcmKeys) << i;
    }

    ASSERT_EQ(actual.dpcmSamples.size(), expected.dpcmSamples.size());
    for (size_t i = 0; i < expected.dpcmSamples.size(); i++) {
        EXPECT_EQ(actual.dpcmSamples[i].name, expected.dpcmSamples[i].name) << i;
        EXPECT_EQ(actual.dpcmSamples[i].data, expected.dpcmSamples[i].data) << i;
        EXPECT_EQ(actual.dpcmSamples[i].rate, expected.dpcmSamples[i].rate) << i;
        EXPECT_EQ(actual.dpcmSamples[i].restLevel, expected.dpcmSamples[i].restLevel) << i;
    }

    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        const auto type = static_cast<ChannelType>(channel);
        const size_t count =
            std::max(expected.patterns[channel].size(), actual.patterns[channel].size());
        for (size_t index = 0; index < count; index++) {
            const Pattern* expectedPattern = expected.findPattern(type, static_cast<uint8_t>(index));
            const Pattern* actualPattern = actual.findPattern(type, static_cast<uint8_t>(index));
            const bool expectedEmpty = expectedPattern == nullptr || expectedPattern->isEmpty();
            const bool actualEmpty = actualPattern == nullptr || actualPattern->isEmpty();
            ASSERT_EQ(actualEmpty, expectedEmpty) << "channel " << channel << " pattern " << index;
            if (!expectedEmpty) {
                EXPECT_EQ(actualPattern->rows, expectedPattern->rows)
                    << "channel " << channel << " pattern " << index;
            }
        }
    }
}

std::string loadError(const std::string& json)
{
    const auto result = SongSerializer::loadFromString(json);
    return result.isErr() ? result.unwrapErr() : std::string("(no error)");
}

/** The example with one piece of text replaced. */
std::string exampleWith(const std::string& from, const std::string& to)
{
    std::string json = EXAMPLE_JSON;
    const size_t position = json.find(from);
    EXPECT_NE(position, std::string::npos) << from;
    if (position != std::string::npos) {
        json.replace(position, from.size(), to);
    }
    return json;
}

} // namespace

TEST(SongSerializerTest, WritesTheDocumentedExample)
{
    EXPECT_EQ(SongSerializer::saveToString(createExampleSong()), EXAMPLE_JSON);
}

TEST(SongSerializerTest, ReadsTheDocumentedExample)
{
    const auto result = SongSerializer::loadFromString(EXAMPLE_JSON);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    expectSameSong(createExampleSong(), result.unwrap());
}

TEST(SongSerializerTest, RoundTrip)
{
    const Song song = createFullSong();
    const std::string json = SongSerializer::saveToString(song);
    const auto result = SongSerializer::loadFromString(json);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr() << "\n" << json;
    expectSameSong(song, result.unwrap());

    // Writing again gives the same text
    EXPECT_EQ(SongSerializer::saveToString(result.unwrap()), json);
}

TEST(SongSerializerTest, WritesEmptyEffectColumnsInTheMiddle)
{
    const std::string json = SongSerializer::saveToString(createFullSong());
    EXPECT_NE(json.find(R"("effects": ["C00", "...", "...", "D1F"])"), std::string::npos) << json;
    EXPECT_NE(json.find(R"("effects": ["...", "F96"])"), std::string::npos) << json;
}

TEST(SongSerializerTest, LoadedPatternsHaveAllRows)
{
    const Song song = SongSerializer::loadFromString(EXAMPLE_JSON).unwrap();
    for (const std::vector<Pattern>& pool : song.patterns) {
        for (const Pattern& pattern : pool) {
            EXPECT_EQ(pattern.rows.size(), song.rowsPerPattern);
        }
    }
    // Frames may refer to patterns that are not written
    EXPECT_EQ(song.findCell(ChannelType::Pulse2, 1, 0), nullptr);
}

TEST(SongSerializerTest, OptionalMembersHaveDefaults)
{
    const auto result = SongSerializer::loadFromString(R"({
        "format": "ocf-tracker-song", "version": 1, "rowsPerPattern": 8,
        "channels": [{ "type": "pulse1" }, { "type": "pulse2" }, { "type": "triangle" },
                     { "type": "noise" }, { "type": "dpcm" }],
        "frames": [[0, 0, 0, 0, 0]]
    })");
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    const Song& song = result.unwrap();
    EXPECT_EQ(song.title, "");
    EXPECT_EQ(song.speed, DEFAULT_SPEED);
    EXPECT_EQ(song.tempo, DEFAULT_TEMPO);
    EXPECT_EQ(song.rowsPerPattern, 8);
    EXPECT_EQ(song.effectColumns[3], 1);
    EXPECT_TRUE(song.instruments.empty());
}

TEST(SongSerializerTest, MissingInstrumentNumbersBecomeEmptyInstruments)
{
    const auto result = SongSerializer::loadFromString(exampleWith(R"("index": 1,
      "name": "Bass")",
                                                                   R"("index": 3,
      "name": "Bass")"));
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    const Song& song = result.unwrap();
    ASSERT_EQ(song.instruments.size(), 4u);
    EXPECT_EQ(song.instruments[1].name, "");
    EXPECT_TRUE(song.instruments[2].getSequence(SequenceType::Volume).isEmpty());
    EXPECT_EQ(song.instruments[3].name, "Bass");
}

TEST(SongSerializerTest, IgnoresUnknownMembers)
{
    const std::string json = exampleWith(
        R"("speed": 6,)",
        R"("speed": 6, "unknown": { "nested": [1, 2] }, "comment": "ignored",)");
    const std::string withSequence =
        json.substr(0, json.find(R"("duty")")) + R"("arpeggio": { "values": [0, 4, 7] }, )" +
        json.substr(json.find(R"("duty")"));
    const auto result = SongSerializer::loadFromString(withSequence);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    expectSameSong(createExampleSong(), result.unwrap());
}

TEST(SongSerializerTest, AcceptsUtf8Bom)
{
    const auto result = SongSerializer::loadFromString(std::string("\xEF\xBB\xBF") + EXAMPLE_JSON);
    EXPECT_TRUE(result.isOk()) << result.unwrapErr();
}

TEST(SongSerializerTest, InvalidJsonReportsTheLocation)
{
    const std::string error = loadError("{\n  \"format\": \"ocf-tracker-song\",\n  oops\n}");
    EXPECT_NE(error.find("invalid JSON at line 3"), std::string::npos) << error;
    EXPECT_NE(loadError("").find("invalid JSON"), std::string::npos);
}

TEST(SongSerializerTest, RejectsOtherFormatsAndNewerVersions)
{
    EXPECT_EQ(loadError("[]"), "the root must be an object");
    EXPECT_EQ(loadError(exampleWith(R"("format": "ocf-tracker-song")", R"("format": "other")")),
              R"(/format: must be "ocf-tracker-song")");
    EXPECT_EQ(loadError(exampleWith(R"("format": "ocf-tracker-song",)", "")),
              "/format: is required");
    EXPECT_EQ(loadError(exampleWith(R"("version": 1)", R"("version": 2)")),
              "/version: version 2 is newer than the supported version 1");
}

TEST(SongSerializerTest, RejectsInvalidValues)
{
    struct Case {
        const char* from;
        const char* to;
        const char* error;
    };
    const Case cases[] = {
        {R"("speed": 6)", R"("speed": 0)", "/speed: must be between 1 and 31"},
        {R"("speed": 6)", R"("speed": 6.5)", "/speed: must be an integer"},
        {R"("speed": 6)", R"("speed": "6")", "/speed: must be an integer"},
        {R"("title": "Example")", R"("title": 1)", "/title: must be a string"},
        {R"("rowsPerPattern": 16,)", "", "/rowsPerPattern: is required"},
        {R"("rowsPerPattern": 16)", R"("rowsPerPattern": 257)",
         "/rowsPerPattern: must be between 1 and 256"},
        {R"(,
    { "type": "dpcm", "effectColumns": 1 })",
         "", "/channels: must have 5 channels"},
        {R"("type": "pulse2")", R"("type": "pulse3")", R"(/channels/1/type: must be "pulse2")"},
        {R"("type": "noise", "effectColumns": 1)", R"("type": "noise", "effectColumns": 5)",
         "/channels/3/effectColumns: must be between 1 and 4"},
        {R"("index": 1,
      "name": "Bass")",
         R"("index": 0,
      "name": "Bass")",
         "/instruments/1/index: duplicate instrument 0"},
        {R"("index": 1,
      "name": "Bass")",
         R"("index": 64,
      "name": "Bass")",
         "/instruments/1/index: must be between 0 and 63"},
        {R"("values": [15], "loop": -1)", R"("values": [16], "loop": -1)",
         "/instruments/1/sequences/volume/values/0: must be between 0 and 15"},
        {R"("values": [2], "loop": -1)", R"("values": [4], "loop": -1)",
         "/instruments/0/sequences/duty/values/0: must be between 0 and 3"},
        {R"("values": [2], "loop": -1)", R"("values": [2], "loop": 1)",
         "/instruments/0/sequences/duty/loop: must be between -1 and 0"},
        {R"("duty": { "values": [2], "loop": -1, "release": -1 })", R"("duty": [2])",
         "/instruments/0/sequences/duty: must be an object"},
        {R"([1, 0, 0, 0, 0])", R"([1, 0, 0, 0])",
         "/frames/1: must be an array of 5 pattern numbers"},
        {R"([1, 0, 0, 0, 0])", R"([1, 0, 0, 0, 256])", "/frames/1/4: must be between 0 and 255"},
        {R"("frames": [
    [0, 0, 0, 0, 0],
    [1, 0, 0, 0, 0]
  ])",
         R"("frames": [])", "/frames: must have 1 to 256 frames"},
        {R"("channel": 2)", R"("channel": 0)", "/patterns/2: duplicate pattern 0 of channel 0"},
        {R"("channel": 2)", R"("channel": 5)", "/patterns/2/channel: must be between 0 and 4"},
        {R"({ "row": 4, "note": "E-4" })", R"({ "row": 0, "note": "E-4" })",
         "/patterns/0/rows/1/row: duplicate row 0"},
        {R"({ "row": 4, "note": "E-4" })", R"({ "row": 16, "note": "E-4" })",
         "/patterns/0/rows/1/row: must be between 0 and 15"},
        {R"("note": "E-4")", R"("note": "H-4")", R"(/patterns/0/rows/1/note: invalid note "H-4")"},
        {R"("note": "E-4")", R"("note": "4-#")", R"(/patterns/0/rows/1/note: invalid note "4-#")"},
        {R"("note": "B-3", "instrument": 0, "volume": 12)",
         R"("note": "B-3", "instrument": 0, "volume": 16)",
         "/patterns/1/rows/0/volume: must be between 0 and 15"},
        {R"("effects": ["B00"])", R"("effects": ["Z00"])",
         R"(/patterns/1/rows/2/effects/0: invalid effect "Z00")"},
        {R"("effects": ["B00"])", R"("effects": ["B00", "...", "...", "...", "C00"])",
         "/patterns/1/rows/2/effects: must have at most 4 effects"},
        {R"("effects": ["B00"])", R"("effects": [0])",
         "/patterns/1/rows/2/effects/0: must be a string"},
    };

    for (const Case& testCase : cases) {
        EXPECT_EQ(loadError(exampleWith(testCase.from, testCase.to)), testCase.error)
            << testCase.from << " -> " << testCase.to;
    }
}

TEST(SongSerializerTest, NoiseNotesAreValidOnTheNoiseChannelOnly)
{
    const std::string json = exampleWith(R"("channel": 2)", R"("channel": 3)");
    EXPECT_EQ(loadError(json), R"(/patterns/2/rows/0/note: invalid note "C-3")");

    const auto result = SongSerializer::loadFromString(exampleWith(R"("channel": 2,
      "index": 0,
      "rows": [
        { "row": 0, "note": "C-3", "instrument": 1 },
        { "row": 8, "note": "G-2" })",
                                                                   R"("channel": 3,
      "index": 0,
      "rows": [
        { "row": 0, "note": "0-#", "instrument": 1 },
        { "row": 8, "note": "F-#" })"));
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    EXPECT_EQ(result.unwrap().findCell(ChannelType::Noise, 0, 8)->note, NOISE_NOTE_MAX);
}

TEST(SongSerializerTest, SaveAndLoadFile)
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "ocf_song_serializer_test";
    std::filesystem::remove_all(directory);
    const std::filesystem::path path = directory / "songs" / u8"曲.json";

    const Song song = createFullSong();
    const auto saved = SongSerializer::save(song, path);
    ASSERT_TRUE(saved.isOk()) << saved.unwrapErr();
    std::filesystem::path temporaryPath = path;
    temporaryPath += ".tmp";
    EXPECT_FALSE(std::filesystem::exists(temporaryPath));

    const auto loaded = SongSerializer::load(path);
    ASSERT_TRUE(loaded.isOk()) << loaded.unwrapErr();
    expectSameSong(song, loaded.unwrap());

    // Saving again replaces the file
    const auto savedAgain = SongSerializer::save(createExampleSong(), path);
    ASSERT_TRUE(savedAgain.isOk()) << savedAgain.unwrapErr();
    std::ifstream file(path, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_EQ(text, EXAMPLE_JSON);
    file.close();

    std::filesystem::remove_all(directory);
}

TEST(SongSerializerTest, LoadMissingFile)
{
    const auto result =
        SongSerializer::load(std::filesystem::temp_directory_path() / "ocf_no_such_song.json");
    ASSERT_TRUE(result.isErr());
    EXPECT_NE(result.unwrapErr().find("cannot open"), std::string::npos);
}

namespace {

// 17 bytes of 0x55
constexpr const char* SAMPLE_17_BYTES = "VVVVVVVVVVVVVVVVVVVVVVU=";

std::string exampleWithDpcm(const std::string& samples, const std::string& keys = "")
{
    std::string json = exampleWith("\"instruments\": [", "\"dpcmSamples\": [" + samples + "],\n  \"instruments\": [");
    if (!keys.empty()) {
        const std::string from = "\"name\": \"Bass\",";
        json.replace(json.find(from), from.size(), from + " \"dpcm\": [" + keys + "],");
    }
    return json;
}

} // namespace

TEST(SongSerializerTest, ReadsDpcmSamplesAndKeys)
{
    const std::string json =
        exampleWithDpcm(std::string(R"({ "index": 1, "name": "Snare", "data": ")") + SAMPLE_17_BYTES + "\" }",
                        R"({ "note": "C-3", "sample": 1, "rate": 9, "loop": true }, { "note": "D-3", "sample": 0 })");
    const auto result = SongSerializer::loadFromString(json);
    ASSERT_TRUE(result.isOk()) << result.unwrapErr();
    const Song& song = result.unwrap();

    ASSERT_EQ(song.dpcmSamples.size(), 2u);
    EXPECT_TRUE(song.dpcmSamples[0].data.empty()); // Missing numbers are empty
    EXPECT_EQ(song.dpcmSamples[1].name, "Snare");
    EXPECT_EQ(song.dpcmSamples[1].data, std::vector<uint8_t>(17, 0x55));
    EXPECT_EQ(song.dpcmSamples[1].rate, DPCM_MAX_RATE);           // Default
    EXPECT_EQ(song.dpcmSamples[1].restLevel, DPCM_DEFAULT_LEVEL); // Default

    const Instrument& bass = song.instruments[1];
    EXPECT_EQ(*bass.findDpcmKey(note(ChannelType::Dpcm, "C-3")), (DpcmKey{1, 9, true}));
    EXPECT_EQ(*bass.findDpcmKey(note(ChannelType::Dpcm, "D-3")), (DpcmKey{0, DPCM_MAX_RATE, false}));
    EXPECT_FALSE(bass.findDpcmKey(note(ChannelType::Dpcm, "E-3"))->isAssigned());
}

TEST(SongSerializerTest, WritesDpcmOnlyWhenUsed)
{
    const std::string json = SongSerializer::saveToString(createExampleSong());
    EXPECT_EQ(json.find("dpcmSamples"), std::string::npos);
    EXPECT_EQ(json.find(R"("dpcm": [)"), std::string::npos);

    const std::string full = SongSerializer::saveToString(createFullSong());
    EXPECT_NE(full.find(R"({ "note": "C-3", "sample": 0, "rate": 13 })"), std::string::npos) << full;
    EXPECT_NE(full.find(R"({ "note": "B-7", "sample": 2, "rate": 0, "loop": true })"), std::string::npos) << full;
    EXPECT_NE(full.find(R"({ "index": 1, "name": "", "rate": 15, "restLevel": 64, "data": "" })"), std::string::npos) << full;
    EXPECT_NE(full.find(R"("data": "/w==")"), std::string::npos) << full;
}

TEST(SongSerializerTest, RejectsInvalidDpcm)
{
    const std::string valid = std::string(R"({ "index": 0, "data": ")") + SAMPLE_17_BYTES + "\" }";
    struct Case {
        std::string json;
        std::string error;
    };
    const Case cases[] = {
        {exampleWithDpcm(R"({ "index": 0, "data": "VVV" })"), "/dpcmSamples/0/data: must be base64"},
        {exampleWithDpcm(R"({ "index": 0, "data": "VV=V" })"), "/dpcmSamples/0/data: must be base64"},
        {exampleWithDpcm(R"({ "index": 0, "data": "VVU*" })"), "/dpcmSamples/0/data: must be base64"},
        {exampleWithDpcm(R"({ "index": 0, "data": "VVU=" })"),
         "/dpcmSamples/0/data: must be 16n + 1 bytes, at most 4081 (not 2)"},
        {exampleWithDpcm(R"({ "index": 0 })"), "/dpcmSamples/0/data: is required"},
        {exampleWithDpcm(R"({ "index": 0, "rate": 16, "data": "" })"),
         "/dpcmSamples/0/rate: must be between 0 and 15"},
        {exampleWithDpcm(R"({ "index": 0, "restLevel": 128, "data": "" })"),
         "/dpcmSamples/0/restLevel: must be between 0 and 127"},
        {exampleWithDpcm(valid + ", " + valid), "/dpcmSamples/1/index: duplicate sample 0"},
        {exampleWithDpcm(valid, R"({ "note": "---", "sample": 0 })"),
         "/instruments/1/dpcm/0/note: invalid note \"---\""},
        {exampleWithDpcm(valid, R"({ "note": "C-3", "sample": 0 }, { "note": "C-3", "sample": 0 })"),
         "/instruments/1/dpcm/1/note: duplicate note C-3"},
        {exampleWithDpcm(valid, R"({ "note": "C-3", "sample": 1 })"),
         "/instruments/1/dpcm/0/sample: sample 1 does not exist"},
        {exampleWithDpcm(valid, R"({ "note": "C-3" })"), "/instruments/1/dpcm/0/sample: is required"},
        {exampleWithDpcm(valid, R"({ "note": "C-3", "sample": 0, "loop": 1 })"),
         "/instruments/1/dpcm/0/loop: must be true or false"},
    };
    for (const Case& c : cases) {
        EXPECT_EQ(loadError(c.json), c.error);
    }
}

TEST(SongSerializerTest, RejectsDpcmSamplesBeyondTheMemory)
{
    Song song = Song::createDefault();
    song.dpcmSamples.resize(5);
    for (DpcmSample& sample : song.dpcmSamples) {
        sample.data.assign(DPCM_MAX_SAMPLE_BYTES, 0x55);
    }
    EXPECT_EQ(loadError(SongSerializer::saveToString(song)),
              "/dpcmSamples: the samples take 20480 bytes, more than 16384");
    song.dpcmSamples.pop_back();
    EXPECT_TRUE(SongSerializer::loadFromString(SongSerializer::saveToString(song)).isOk());
}
