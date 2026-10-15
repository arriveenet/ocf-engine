// SPDX-License-Identifier: MIT
#include "ocf/audio/tracker/SongSerializer.h"

#include "ocf/audio/tracker/SongNotation.h"
#include "ocf/core/base64.h"

#include <yyjson.h>

#include <bitset>
#include <climits>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

namespace ocf::audio::tracker {

namespace {

struct MutableDocumentDeleter {
    void operator()(yyjson_mut_doc* document) const { yyjson_mut_doc_free(document); }
};

struct DocumentDeleter {
    void operator()(yyjson_doc* document) const { yyjson_doc_free(document); }
};

using MutableDocument = std::unique_ptr<yyjson_mut_doc, MutableDocumentDeleter>;
using Document = std::unique_ptr<yyjson_doc, DocumentDeleter>;

/** Range of the values of each sequence type. */
struct SequenceRange {
    int min;
    int max;
};

SequenceRange getSequenceRange(SequenceType type)
{
    switch (type) {
    case SequenceType::Volume: return {0, MAX_VOLUME};
    default:                   return {0, 3};
    }
}

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

//--------------------------------------------------------------------------------------------------
// Writing
//--------------------------------------------------------------------------------------------------

void addString(yyjson_mut_doc* document, yyjson_mut_val* object, const char* key,
               std::string_view value)
{
    yyjson_mut_obj_add_strncpy(document, object, key, value.data(), value.size());
}

yyjson_mut_val* createSequence(yyjson_mut_doc* document, const Sequence& sequence)
{
    yyjson_mut_val* object = yyjson_mut_obj(document);
    yyjson_mut_val* values = yyjson_mut_obj_add_arr(document, object, "values");
    for (int8_t value : sequence.values) {
        yyjson_mut_arr_add_int(document, values, value);
    }
    yyjson_mut_obj_add_int(document, object, "loop", sequence.loopPoint);
    yyjson_mut_obj_add_int(document, object, "release", sequence.releasePoint);
    return object;
}

yyjson_mut_val* createRow(yyjson_mut_doc* document, ChannelType channel, const Cell& cell,
                          size_t row)
{
    yyjson_mut_val* object = yyjson_mut_obj(document);
    yyjson_mut_obj_add_uint(document, object, "row", row);
    if (cell.note != NOTE_NONE) {
        addString(document, object, "note", noteToString(channel, cell.note));
    }
    if (cell.instrument != VALUE_NONE) {
        yyjson_mut_obj_add_uint(document, object, "instrument", cell.instrument);
    }
    if (cell.volume != VALUE_NONE) {
        yyjson_mut_obj_add_uint(document, object, "volume", cell.volume);
    }

    // Empty columns in the middle are "...", and the empty columns at the end are left out
    int lastEffect = -1;
    for (int i = 0; i < static_cast<int>(MAX_EFFECT_COLUMNS); i++) {
        if (cell.effects[i].type != EffectType::None) {
            lastEffect = i;
        }
    }
    if (lastEffect >= 0) {
        yyjson_mut_val* effects = yyjson_mut_obj_add_arr(document, object, "effects");
        for (int i = 0; i <= lastEffect; i++) {
            const std::string text = effectToString(cell.effects[i]);
            yyjson_mut_arr_add_strncpy(document, effects, text.data(), text.size());
        }
    }
    return object;
}

MutableDocument createDocument(const Song& song)
{
    MutableDocument owner(yyjson_mut_doc_new(nullptr));
    yyjson_mut_doc* document = owner.get();
    yyjson_mut_val* root = yyjson_mut_obj(document);
    yyjson_mut_doc_set_root(document, root);

    yyjson_mut_obj_add_str(document, root, "format", SongSerializer::FORMAT_NAME);
    yyjson_mut_obj_add_int(document, root, "version", SongSerializer::FORMAT_VERSION);
    addString(document, root, "title", song.title);
    addString(document, root, "author", song.author);
    yyjson_mut_obj_add_uint(document, root, "speed", song.speed);
    yyjson_mut_obj_add_uint(document, root, "tempo", song.tempo);
    yyjson_mut_obj_add_uint(document, root, "rowsPerPattern", song.rowsPerPattern);

    yyjson_mut_val* channels = yyjson_mut_obj_add_arr(document, root, "channels");
    for (size_t i = 0; i < CHANNEL_COUNT; i++) {
        yyjson_mut_val* channel = yyjson_mut_arr_add_obj(document, channels);
        addString(document, channel, "type", channelTypeToString(static_cast<ChannelType>(i)));
        yyjson_mut_obj_add_uint(document, channel, "effectColumns", song.effectColumns[i]);
    }

    yyjson_mut_val* instruments = yyjson_mut_obj_add_arr(document, root, "instruments");
    for (size_t i = 0; i < song.instruments.size(); i++) {
        const Instrument& instrument = song.instruments[i];
        yyjson_mut_val* object = yyjson_mut_arr_add_obj(document, instruments);
        yyjson_mut_obj_add_uint(document, object, "index", i);
        addString(document, object, "name", instrument.name);

        yyjson_mut_val* sequences = yyjson_mut_obj_add_obj(document, object, "sequences");
        for (size_t type = 0; type < SEQUENCE_TYPE_COUNT; type++) {
            const Sequence& sequence = instrument.sequences[type];
            if (!sequence.isEmpty()) {
                // The names are string literals, so the keys need not be copied
                const char* key = sequenceTypeToString(static_cast<SequenceType>(type)).data();
                yyjson_mut_obj_add_val(document, sequences, key, createSequence(document, sequence));
            }
        }

        // Only the assigned keys, and only when there are any
        const auto& keys = instrument.dpcmKeys;
        if (std::any_of(keys.begin(), keys.end(), [](const DpcmKey& key) { return key.isAssigned(); })) {
            yyjson_mut_val* dpcm = yyjson_mut_obj_add_arr(document, object, "dpcm");
            for (size_t k = 0; k < keys.size(); k++) {
                if (!keys[k].isAssigned()) {
                    continue;
                }
                yyjson_mut_val* key = yyjson_mut_arr_add_obj(document, dpcm);
                addString(document, key, "note",
                          noteToString(ChannelType::Dpcm, static_cast<uint8_t>(NOTE_MIN + k)));
                yyjson_mut_obj_add_uint(document, key, "sample", keys[k].sample);
                yyjson_mut_obj_add_uint(document, key, "rate", keys[k].rate);
                if (keys[k].loop) {
                    yyjson_mut_obj_add_bool(document, key, "loop", true);
                }
            }
        }
    }

    yyjson_mut_val* frames = yyjson_mut_obj_add_arr(document, root, "frames");
    for (const Frame& frame : song.frames) {
        yyjson_mut_val* patterns = yyjson_mut_arr_add_arr(document, frames);
        for (uint8_t pattern : frame) {
            yyjson_mut_arr_add_uint(document, patterns, pattern);
        }
    }

    yyjson_mut_val* patterns = yyjson_mut_obj_add_arr(document, root, "patterns");
    for (size_t channel = 0; channel < CHANNEL_COUNT; channel++) {
        const auto type = static_cast<ChannelType>(channel);
        const std::vector<Pattern>& pool = song.patterns[channel];
        for (size_t index = 0; index < pool.size(); index++) {
            if (pool[index].isEmpty()) {
                continue;
            }
            yyjson_mut_val* object = yyjson_mut_arr_add_obj(document, patterns);
            yyjson_mut_obj_add_uint(document, object, "channel", channel);
            yyjson_mut_obj_add_uint(document, object, "index", index);
            yyjson_mut_val* rows = yyjson_mut_obj_add_arr(document, object, "rows");
            for (size_t row = 0; row < pool[index].rows.size(); row++) {
                const Cell& cell = pool[index].rows[row];
                if (!cell.isEmpty()) {
                    yyjson_mut_arr_append(rows, createRow(document, type, cell, row));
                }
            }
        }
    }

    // Last, since the data is long
    if (!song.dpcmSamples.empty()) {
        yyjson_mut_val* samples = yyjson_mut_obj_add_arr(document, root, "dpcmSamples");
        for (size_t i = 0; i < song.dpcmSamples.size(); i++) {
            const DpcmSample& sample = song.dpcmSamples[i];
            yyjson_mut_val* object = yyjson_mut_arr_add_obj(document, samples);
            yyjson_mut_obj_add_uint(document, object, "index", i);
            addString(document, object, "name", sample.name);
            yyjson_mut_obj_add_uint(document, object, "rate", sample.rate);
            yyjson_mut_obj_add_uint(document, object, "restLevel", sample.restLevel);
            addString(document, object, "data", encodeBase64(sample.data));
        }
    }

    return owner;
}

//--------------------------------------------------------------------------------------------------
// Layout of the written JSON: a container is written on one line when it holds only scalars, or
// when it is an object holding only scalars and arrays of scalars (a row, a frame, a sequence).
// Everything else has one member per line, indented by two spaces.
//--------------------------------------------------------------------------------------------------

bool isScalarArray(yyjson_mut_val* value)
{
    if (!yyjson_mut_is_arr(value)) {
        return false;
    }
    size_t index = 0;
    size_t max = 0;
    yyjson_mut_val* item = nullptr;
    yyjson_mut_arr_foreach(value, index, max, item)
    {
        if (yyjson_mut_is_ctn(item)) {
            return false;
        }
    }
    return true;
}

bool isWrittenOnOneLine(yyjson_mut_val* value)
{
    if (yyjson_mut_is_arr(value)) {
        return isScalarArray(value);
    }
    size_t index = 0;
    size_t max = 0;
    yyjson_mut_val* key = nullptr;
    yyjson_mut_val* member = nullptr;
    yyjson_mut_obj_foreach(value, index, max, key, member)
    {
        if (yyjson_mut_is_ctn(member) && !isScalarArray(member)) {
            return false;
        }
    }
    return true;
}

void appendScalar(std::string& output, const yyjson_mut_val* value)
{
    // Strings are escaped by yyjson. Invalid UTF-8 is written as is rather than failing.
    size_t length = 0;
    char* text = yyjson_mut_val_write(value, YYJSON_WRITE_ALLOW_INVALID_UNICODE, &length);
    if (text != nullptr) {
        output.append(text, length);
        std::free(text);
    }
}

void appendOneLine(std::string& output, yyjson_mut_val* value)
{
    if (yyjson_mut_is_arr(value)) {
        output += '[';
        size_t index = 0;
        size_t max = 0;
        yyjson_mut_val* item = nullptr;
        yyjson_mut_arr_foreach(value, index, max, item)
        {
            if (index > 0) {
                output += ", ";
            }
            appendOneLine(output, item);
        }
        output += ']';
    }
    else if (yyjson_mut_is_obj(value)) {
        if (yyjson_mut_obj_size(value) == 0) {
            output += "{}";
            return;
        }
        output += "{ ";
        size_t index = 0;
        size_t max = 0;
        yyjson_mut_val* key = nullptr;
        yyjson_mut_val* member = nullptr;
        yyjson_mut_obj_foreach(value, index, max, key, member)
        {
            if (index > 0) {
                output += ", ";
            }
            appendScalar(output, key);
            output += ": ";
            appendOneLine(output, member);
        }
        output += " }";
    }
    else {
        appendScalar(output, value);
    }
}

void appendValue(std::string& output, yyjson_mut_val* value, size_t indent)
{
    if (!yyjson_mut_is_ctn(value) || isWrittenOnOneLine(value)) {
        appendOneLine(output, value);
        return;
    }

    const bool isArray = yyjson_mut_is_arr(value);
    output += isArray ? "[\n" : "{\n";
    const size_t count = isArray ? yyjson_mut_arr_size(value) : yyjson_mut_obj_size(value);

    size_t index = 0;
    size_t max = 0;
    yyjson_mut_val* key = nullptr;
    yyjson_mut_val* member = nullptr;
    auto appendMember = [&](yyjson_mut_val* memberKey, yyjson_mut_val* memberValue) {
        output.append(indent + 2, ' ');
        if (memberKey != nullptr) {
            appendScalar(output, memberKey);
            output += ": ";
        }
        appendValue(output, memberValue, indent + 2);
        output += (index + 1 < count) ? ",\n" : "\n";
    };
    if (isArray) {
        yyjson_mut_arr_foreach(value, index, max, member)
        {
            appendMember(nullptr, member);
        }
    }
    else {
        yyjson_mut_obj_foreach(value, index, max, key, member)
        {
            appendMember(key, member);
        }
    }

    output.append(indent, ' ');
    output += isArray ? ']' : '}';
}

//--------------------------------------------------------------------------------------------------
// Reading
//--------------------------------------------------------------------------------------------------

/** Reads a song, keeping the first error. Every function returns false on error. */
class SongReader {
public:
    bool readSong(yyjson_val* root, Song& song);

    const std::string& getError() const { return m_error; }

private:
    bool readChannels(yyjson_val* root, Song& song);
    bool readDpcmSamples(yyjson_val* root, Song& song);
    bool readInstruments(yyjson_val* root, Song& song);
    bool readDpcmKeys(yyjson_val* value, const std::string& path, const Song& song,
                      Instrument& instrument);
    bool readSequence(yyjson_val* value, const std::string& path, SequenceType type,
                      Sequence& sequence);
    bool readFrames(yyjson_val* root, Song& song);
    bool readPatterns(yyjson_val* root, Song& song);
    bool readRow(yyjson_val* value, const std::string& path, ChannelType channel, Song& song,
                 Pattern& pattern, std::vector<bool>& rowsSeen);

    bool fail(const std::string& path, std::string_view message);

    bool readIntValue(yyjson_val* value, const std::string& path, int min, int max, int& result);

    // The members are optional unless `required`. An absent member leaves `result` unchanged.
    bool readInt(yyjson_val* object, const char* key, const std::string& path, int min, int max,
                 int& result, bool required = false);
    bool readString(yyjson_val* object, const char* key, const std::string& path,
                    std::string& result, bool required = false);
    bool readBool(yyjson_val* object, const char* key, const std::string& path, bool& result);
    bool getArray(yyjson_val* object, const char* key, const std::string& path,
                  yyjson_val*& result, bool required = false);
    bool getObject(yyjson_val* object, const char* key, const std::string& path,
                   yyjson_val*& result, bool required = false);

    std::string m_error;
};

std::string memberPath(const std::string& path, std::string_view key)
{
    return std::format("{}/{}", path, key);
}

std::string itemPath(const std::string& path, size_t index)
{
    return std::format("{}/{}", path, index);
}

bool SongReader::fail(const std::string& path, std::string_view message)
{
    if (m_error.empty()) {
        m_error = path.empty() ? std::string(message) : std::format("{}: {}", path, message);
    }
    return false;
}

bool SongReader::readIntValue(yyjson_val* value, const std::string& path, int min, int max,
                              int& result)
{
    std::optional<int64_t> number;
    if (yyjson_is_sint(value)) {
        number = yyjson_get_sint(value);
    }
    else if (yyjson_is_uint(value)) {
        const uint64_t unsignedNumber = yyjson_get_uint(value);
        number = unsignedNumber > static_cast<uint64_t>(INT64_MAX) ? INT64_MAX
                                                                    : static_cast<int64_t>(unsignedNumber);
    }

    if (!number) {
        return fail(path, "must be an integer");
    }
    if (*number < min || *number > max) {
        return fail(path, std::format("must be between {} and {}", min, max));
    }
    result = static_cast<int>(*number);
    return true;
}

bool SongReader::readInt(yyjson_val* object, const char* key, const std::string& path, int min,
                         int max, int& result, bool required)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    return readIntValue(member, memberPath(path, key), min, max, result);
}

bool SongReader::readString(yyjson_val* object, const char* key, const std::string& path,
                            std::string& result, bool required)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    if (!yyjson_is_str(member)) {
        return fail(memberPath(path, key), "must be a string");
    }
    result.assign(yyjson_get_str(member), yyjson_get_len(member));
    return true;
}

bool SongReader::readBool(yyjson_val* object, const char* key, const std::string& path, bool& result)
{
    yyjson_val* member = yyjson_obj_get(object, key);
    if (member == nullptr) {
        return true;
    }
    if (!yyjson_is_bool(member)) {
        return fail(memberPath(path, key), "must be true or false");
    }
    result = yyjson_get_bool(member);
    return true;
}

bool SongReader::getArray(yyjson_val* object, const char* key, const std::string& path,
                          yyjson_val*& result, bool required)
{
    result = yyjson_obj_get(object, key);
    if (result == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    if (!yyjson_is_arr(result)) {
        return fail(memberPath(path, key), "must be an array");
    }
    return true;
}

bool SongReader::getObject(yyjson_val* object, const char* key, const std::string& path,
                           yyjson_val*& result, bool required)
{
    result = yyjson_obj_get(object, key);
    if (result == nullptr) {
        return required ? fail(memberPath(path, key), "is required") : true;
    }
    if (!yyjson_is_obj(result)) {
        return fail(memberPath(path, key), "must be an object");
    }
    return true;
}

bool SongReader::readSong(yyjson_val* root, Song& song)
{
    const std::string path;
    if (!yyjson_is_obj(root)) {
        return fail(path, "the root must be an object");
    }

    std::string format;
    if (!readString(root, "format", path, format, true)) {
        return false;
    }
    if (format != SongSerializer::FORMAT_NAME) {
        return fail("/format", std::format("must be \"{}\"", SongSerializer::FORMAT_NAME));
    }

    int version = 0;
    if (!readInt(root, "version", path, 1, INT_MAX, version, true)) {
        return false;
    }
    if (version > SongSerializer::FORMAT_VERSION) {
        return fail("/version", std::format("version {} is newer than the supported version {}",
                                            version, SongSerializer::FORMAT_VERSION));
    }

    int speed = DEFAULT_SPEED;
    int tempo = DEFAULT_TEMPO;
    int rows = DEFAULT_ROWS_PER_PATTERN;
    if (!readString(root, "title", path, song.title) ||
        !readString(root, "author", path, song.author) ||
        !readInt(root, "speed", path, MIN_SPEED, MAX_SPEED, speed) ||
        !readInt(root, "tempo", path, MIN_TEMPO, MAX_TEMPO, tempo) ||
        !readInt(root, "rowsPerPattern", path, 1, static_cast<int>(MAX_ROWS), rows, true)) {
        return false;
    }
    song.speed = static_cast<uint8_t>(speed);
    song.tempo = static_cast<uint8_t>(tempo);
    // Set before the patterns are added, since they are created with this many rows
    song.rowsPerPattern = static_cast<uint16_t>(rows);

    // The samples first, since the instruments refer to them
    return readChannels(root, song) && readDpcmSamples(root, song) && readInstruments(root, song) &&
           readFrames(root, song) && readPatterns(root, song);
}

bool SongReader::readChannels(yyjson_val* root, Song& song)
{
    yyjson_val* channels = nullptr;
    if (!getArray(root, "channels", "", channels, true)) {
        return false;
    }
    if (yyjson_arr_size(channels) != CHANNEL_COUNT) {
        return fail("/channels", std::format("must have {} channels", CHANNEL_COUNT));
    }

    size_t index = 0;
    size_t max = 0;
    yyjson_val* channel = nullptr;
    yyjson_arr_foreach(channels, index, max, channel)
    {
        const std::string path = itemPath("/channels", index);
        if (!yyjson_is_obj(channel)) {
            return fail(path, "must be an object");
        }

        std::string type;
        if (!readString(channel, "type", path, type, true)) {
            return false;
        }
        const std::string_view expected = channelTypeToString(static_cast<ChannelType>(index));
        if (type != expected) {
            return fail(memberPath(path, "type"), std::format("must be \"{}\"", expected));
        }

        int effectColumns = 1;
        if (!readInt(channel, "effectColumns", path, 1, static_cast<int>(MAX_EFFECT_COLUMNS),
                     effectColumns)) {
            return false;
        }
        song.effectColumns[index] = static_cast<uint8_t>(effectColumns);
    }
    return true;
}

bool SongReader::readDpcmSamples(yyjson_val* root, Song& song)
{
    yyjson_val* samples = nullptr;
    if (!getArray(root, "dpcmSamples", "", samples)) {
        return false;
    }
    if (samples == nullptr) {
        return true;
    }

    std::bitset<MAX_DPCM_SAMPLES> seen;
    size_t index = 0;
    size_t max = 0;
    yyjson_val* value = nullptr;
    yyjson_arr_foreach(samples, index, max, value)
    {
        const std::string path = itemPath("/dpcmSamples", index);
        if (!yyjson_is_obj(value)) {
            return fail(path, "must be an object");
        }

        int number = 0;
        if (!readInt(value, "index", path, 0, static_cast<int>(MAX_DPCM_SAMPLES) - 1, number, true)) {
            return false;
        }
        if (seen[number]) {
            return fail(memberPath(path, "index"), std::format("duplicate sample {}", number));
        }
        seen[number] = true;

        // Missing numbers become empty samples
        if (song.dpcmSamples.size() <= static_cast<size_t>(number)) {
            song.dpcmSamples.resize(number + 1);
        }
        DpcmSample& sample = song.dpcmSamples[number];
        int rate = DPCM_MAX_RATE;
        int restLevel = DPCM_DEFAULT_LEVEL;
        std::string text;
        if (!readString(value, "name", path, sample.name) ||
            !readInt(value, "rate", path, 0, DPCM_MAX_RATE, rate) ||
            !readInt(value, "restLevel", path, 0, DPCM_LEVEL_MAX, restLevel) ||
            !readString(value, "data", path, text, true)) {
            return false;
        }
        sample.rate = static_cast<uint8_t>(rate);
        sample.restLevel = static_cast<uint8_t>(restLevel);

        std::optional<std::vector<uint8_t>> data = decodeBase64(text);
        if (!data) {
            return fail(memberPath(path, "data"), "must be base64");
        }
        if (!data->empty() && (data->size() > DPCM_MAX_SAMPLE_BYTES || (data->size() - 1) % 16 != 0)) {
            return fail(memberPath(path, "data"),
                        std::format("must be 16n + 1 bytes, at most {} (not {})",
                                    DPCM_MAX_SAMPLE_BYTES, data->size()));
        }
        sample.data = std::move(*data);
    }

    if (song.getDpcmMemoryUsed() > DPCM_MEMORY_SIZE) {
        return fail("/dpcmSamples", std::format("the samples take {} bytes, more than {}",
                                                song.getDpcmMemoryUsed(), DPCM_MEMORY_SIZE));
    }
    return true;
}

bool SongReader::readInstruments(yyjson_val* root, Song& song)
{
    yyjson_val* instruments = nullptr;
    if (!getArray(root, "instruments", "", instruments)) {
        return false;
    }
    if (instruments == nullptr) {
        return true;
    }

    std::bitset<MAX_INSTRUMENTS> seen;
    size_t index = 0;
    size_t max = 0;
    yyjson_val* value = nullptr;
    yyjson_arr_foreach(instruments, index, max, value)
    {
        const std::string path = itemPath("/instruments", index);
        if (!yyjson_is_obj(value)) {
            return fail(path, "must be an object");
        }

        int number = 0;
        if (!readInt(value, "index", path, 0, static_cast<int>(MAX_INSTRUMENTS) - 1, number, true)) {
            return false;
        }
        if (seen[number]) {
            return fail(memberPath(path, "index"), std::format("duplicate instrument {}", number));
        }
        seen[number] = true;

        // Missing numbers become empty instruments
        if (song.instruments.size() <= static_cast<size_t>(number)) {
            song.instruments.resize(number + 1);
        }
        Instrument& instrument = song.instruments[number];
        if (!readString(value, "name", path, instrument.name) ||
            !readDpcmKeys(value, path, song, instrument)) {
            return false;
        }

        yyjson_val* sequences = nullptr;
        if (!getObject(value, "sequences", path, sequences)) {
            return false;
        }
        if (sequences == nullptr) {
            continue;
        }
        // Unknown sequence types are ignored, for files from newer versions
        for (size_t type = 0; type < SEQUENCE_TYPE_COUNT; type++) {
            const auto sequenceType = static_cast<SequenceType>(type);
            const std::string_view name = sequenceTypeToString(sequenceType);
            yyjson_val* sequence = yyjson_obj_getn(sequences, name.data(), name.size());
            if (sequence != nullptr &&
                !readSequence(sequence, memberPath(memberPath(path, "sequences"), name),
                              sequenceType, instrument.getSequence(sequenceType))) {
                return false;
            }
        }
    }
    return true;
}

bool SongReader::readDpcmKeys(yyjson_val* value, const std::string& path, const Song& song,
                              Instrument& instrument)
{
    yyjson_val* keys = nullptr;
    if (!getArray(value, "dpcm", path, keys)) {
        return false;
    }
    if (keys == nullptr) {
        return true;
    }

    const std::string keysPath = memberPath(path, "dpcm");
    size_t index = 0;
    size_t max = 0;
    yyjson_val* item = nullptr;
    yyjson_arr_foreach(keys, index, max, item)
    {
        const std::string keyPath = itemPath(keysPath, index);
        if (!yyjson_is_obj(item)) {
            return fail(keyPath, "must be an object");
        }

        std::string noteText;
        if (!readString(item, "note", keyPath, noteText, true)) {
            return false;
        }
        const std::optional<uint8_t> note = noteFromString(ChannelType::Dpcm, noteText);
        DpcmKey* key = note ? instrument.findDpcmKey(*note) : nullptr;
        if (key == nullptr) {
            return fail(memberPath(keyPath, "note"), std::format("invalid note \"{}\"", noteText));
        }
        if (key->isAssigned()) {
            return fail(memberPath(keyPath, "note"), std::format("duplicate note {}", noteText));
        }

        int sample = 0;
        int rate = DPCM_MAX_RATE;
        bool loop = false;
        if (!readInt(item, "sample", keyPath, 0, static_cast<int>(MAX_DPCM_SAMPLES) - 1, sample,
                     true) ||
            !readInt(item, "rate", keyPath, 0, DPCM_MAX_RATE, rate) ||
            !readBool(item, "loop", keyPath, loop)) {
            return false;
        }
        if (static_cast<size_t>(sample) >= song.dpcmSamples.size()) {
            return fail(memberPath(keyPath, "sample"), std::format("sample {} does not exist", sample));
        }
        *key = DpcmKey{static_cast<uint8_t>(sample), static_cast<uint8_t>(rate), loop};
    }
    return true;
}

bool SongReader::readSequence(yyjson_val* value, const std::string& path, SequenceType type,
                              Sequence& sequence)
{
    if (!yyjson_is_obj(value)) {
        return fail(path, "must be an object");
    }

    yyjson_val* values = nullptr;
    if (!getArray(value, "values", path, values, true)) {
        return false;
    }
    if (yyjson_arr_size(values) > MAX_SEQUENCE_LENGTH) {
        return fail(memberPath(path, "values"),
                    std::format("must have at most {} values", MAX_SEQUENCE_LENGTH));
    }

    const SequenceRange range = getSequenceRange(type);
    size_t index = 0;
    size_t max = 0;
    yyjson_val* item = nullptr;
    yyjson_arr_foreach(values, index, max, item)
    {
        int number = 0;
        if (!readIntValue(item, itemPath(memberPath(path, "values"), index), range.min,
                          range.max, number)) {
            return false;
        }
        sequence.values.push_back(static_cast<int8_t>(number));
    }

    const int last = static_cast<int>(sequence.values.size()) - 1;
    int loop = -1;
    int release = -1;
    if (!readInt(value, "loop", path, -1, last, loop) ||
        !readInt(value, "release", path, -1, last, release)) {
        return false;
    }
    sequence.loopPoint = static_cast<int16_t>(loop);
    sequence.releasePoint = static_cast<int16_t>(release);
    return true;
}

bool SongReader::readFrames(yyjson_val* root, Song& song)
{
    yyjson_val* frames = nullptr;
    if (!getArray(root, "frames", "", frames, true)) {
        return false;
    }
    const size_t frameCount = yyjson_arr_size(frames);
    if (frameCount == 0 || frameCount > MAX_FRAMES) {
        return fail("/frames", std::format("must have 1 to {} frames", MAX_FRAMES));
    }

    size_t index = 0;
    size_t max = 0;
    yyjson_val* value = nullptr;
    yyjson_arr_foreach(frames, index, max, value)
    {
        const std::string path = itemPath("/frames", index);
        if (!yyjson_is_arr(value) || yyjson_arr_size(value) != CHANNEL_COUNT) {
            return fail(path, std::format("must be an array of {} pattern numbers", CHANNEL_COUNT));
        }

        Frame frame{};
        size_t channel = 0;
        size_t channelMax = 0;
        yyjson_val* pattern = nullptr;
        yyjson_arr_foreach(value, channel, channelMax, pattern)
        {
            int number = 0;
            if (!readIntValue(pattern, itemPath(path, channel), 0,
                              static_cast<int>(MAX_PATTERNS) - 1, number)) {
                return false;
            }
            frame[channel] = static_cast<uint8_t>(number);
        }
        song.frames.push_back(frame);
    }
    return true;
}

bool SongReader::readPatterns(yyjson_val* root, Song& song)
{
    yyjson_val* patterns = nullptr;
    if (!getArray(root, "patterns", "", patterns)) {
        return false;
    }
    if (patterns == nullptr) {
        return true;
    }

    std::array<std::bitset<MAX_PATTERNS>, CHANNEL_COUNT> seen;
    size_t index = 0;
    size_t max = 0;
    yyjson_val* value = nullptr;
    yyjson_arr_foreach(patterns, index, max, value)
    {
        const std::string path = itemPath("/patterns", index);
        if (!yyjson_is_obj(value)) {
            return fail(path, "must be an object");
        }

        int channel = 0;
        int number = 0;
        yyjson_val* rows = nullptr;
        if (!readInt(value, "channel", path, 0, static_cast<int>(CHANNEL_COUNT) - 1, channel,
                     true) ||
            !readInt(value, "index", path, 0, static_cast<int>(MAX_PATTERNS) - 1, number, true) ||
            !getArray(value, "rows", path, rows, true)) {
            return false;
        }
        if (seen[channel][number]) {
            return fail(path, std::format("duplicate pattern {} of channel {}", number, channel));
        }
        seen[channel][number] = true;

        const auto type = static_cast<ChannelType>(channel);
        Pattern& pattern = song.getPattern(type, static_cast<uint8_t>(number));
        std::vector<bool> rowsSeen(song.rowsPerPattern);

        size_t rowIndex = 0;
        size_t rowMax = 0;
        yyjson_val* row = nullptr;
        yyjson_arr_foreach(rows, rowIndex, rowMax, row)
        {
            if (!readRow(row, itemPath(memberPath(path, "rows"), rowIndex), type, song, pattern,
                         rowsSeen)) {
                return false;
            }
        }
    }
    return true;
}

bool SongReader::readRow(yyjson_val* value, const std::string& path, ChannelType channel,
                         Song& song, Pattern& pattern, std::vector<bool>& rowsSeen)
{
    if (!yyjson_is_obj(value)) {
        return fail(path, "must be an object");
    }

    int row = 0;
    if (!readInt(value, "row", path, 0, song.rowsPerPattern - 1, row, true)) {
        return false;
    }
    if (rowsSeen[row]) {
        return fail(memberPath(path, "row"), std::format("duplicate row {}", row));
    }
    rowsSeen[row] = true;
    Cell& cell = pattern.rows[row];

    if (yyjson_val* note = yyjson_obj_get(value, "note")) {
        if (!yyjson_is_str(note)) {
            return fail(memberPath(path, "note"), "must be a string");
        }
        const std::string_view text(yyjson_get_str(note), yyjson_get_len(note));
        const std::optional<uint8_t> noteValue = noteFromString(channel, text);
        if (!noteValue) {
            return fail(memberPath(path, "note"), std::format("invalid note \"{}\"", text));
        }
        cell.note = *noteValue;
    }

    int instrument = VALUE_NONE;
    int volume = VALUE_NONE;
    if (!readInt(value, "instrument", path, 0, static_cast<int>(MAX_INSTRUMENTS) - 1,
                 instrument) ||
        !readInt(value, "volume", path, 0, MAX_VOLUME, volume)) {
        return false;
    }
    cell.instrument = static_cast<uint8_t>(instrument);
    cell.volume = static_cast<uint8_t>(volume);

    yyjson_val* effects = nullptr;
    if (!getArray(value, "effects", path, effects)) {
        return false;
    }
    if (effects == nullptr) {
        return true;
    }
    const std::string effectsPath = memberPath(path, "effects");
    if (yyjson_arr_size(effects) > MAX_EFFECT_COLUMNS) {
        return fail(effectsPath, std::format("must have at most {} effects", MAX_EFFECT_COLUMNS));
    }

    size_t index = 0;
    size_t max = 0;
    yyjson_val* item = nullptr;
    yyjson_arr_foreach(effects, index, max, item)
    {
        if (!yyjson_is_str(item)) {
            return fail(itemPath(effectsPath, index), "must be a string");
        }
        const std::string_view text(yyjson_get_str(item), yyjson_get_len(item));
        const std::optional<Effect> effect = effectFromString(text);
        if (!effect) {
            return fail(itemPath(effectsPath, index), std::format("invalid effect \"{}\"", text));
        }
        cell.effects[index] = *effect;
    }
    return true;
}

} // namespace

std::string SongSerializer::saveToString(const Song& song)
{
    const MutableDocument document = createDocument(song);
    std::string output;
    appendValue(output, yyjson_mut_doc_get_root(document.get()), 0);
    output += '\n';
    return output;
}

Result<Song, std::string> SongSerializer::loadFromString(std::string_view json)
{
    // A UTF-8 BOM is accepted, since some editors add one
    const yyjson_read_flag flags = YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_INVALID_UNICODE;
    yyjson_read_err error{};
    // The text is not modified without YYJSON_READ_INSITU
    const Document document(
        yyjson_read_opts(const_cast<char*>(json.data()), json.size(), flags, nullptr, &error));
    if (!document) {
        size_t line = 0;
        size_t column = 0;
        size_t character = 0;
        if (yyjson_locate_pos(json.data(), json.size(), error.pos, &line, &column, &character)) {
            return Result<Song, std::string>::Err(std::format(
                "invalid JSON at line {}, column {}: {}", line, column, error.msg));
        }
        return Result<Song, std::string>::Err(std::format("invalid JSON: {}", error.msg));
    }

    Song song;
    SongReader reader;
    if (!reader.readSong(yyjson_doc_get_root(document.get()), song)) {
        return Result<Song, std::string>::Err(reader.getError());
    }
    return Result<Song, std::string>::Ok(std::move(song));
}

Result<void, std::string> SongSerializer::save(const Song& song, const std::filesystem::path& path)
{
    const std::string json = SongSerializer::saveToString(song);

    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
    }

    std::filesystem::path temporaryPath = path;
    temporaryPath += ".tmp";
    {
        std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            return Result<void, std::string>::Err(
                std::format("cannot open {} for writing", toUtf8(path)));
        }
        file.write(json.data(), static_cast<std::streamsize>(json.size()));
        file.close();
        if (!file) {
            std::filesystem::remove(temporaryPath, error);
            return Result<void, std::string>::Err(std::format("cannot write {}", toUtf8(path)));
        }
    }

    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        const std::string message = error.message();
        std::filesystem::remove(temporaryPath, error);
        return Result<void, std::string>::Err(
            std::format("cannot replace {}: {}", toUtf8(path), message));
    }
    return Result<void, std::string>::Ok();
}

Result<Song, std::string> SongSerializer::load(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return Result<Song, std::string>::Err(std::format("cannot open {}", toUtf8(path)));
    }
    const std::string json((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    if (file.bad()) {
        return Result<Song, std::string>::Err(std::format("cannot read {}", toUtf8(path)));
    }
    return SongSerializer::loadFromString(json);
}

} // namespace ocf::audio::tracker
