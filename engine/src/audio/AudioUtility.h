#pragma once

#include "ocf/audio/AudioEnums.h"

#include <miniaudio.h>

namespace ocf::audio {

namespace AudioUtility {

constexpr ma_format getMiniaudioFormat(AudioFormat format)
{
    switch (format) {
    case AudioFormat::U8:   return ma_format_u8;
    case AudioFormat::S16:  return ma_format_s16;
    case AudioFormat::S24:  return ma_format_s24;
    case AudioFormat::S32:  return ma_format_s32;
    case AudioFormat::F32:  return ma_format_f32;
    default:
        return ma_format_unknown;
    }
}

constexpr AudioFormat getAudioFormat(ma_format format)
{
    switch (format) {
    case ma_format_u8:   return AudioFormat::U8;
    case ma_format_s16:  return AudioFormat::S16;
    case ma_format_s24:  return AudioFormat::S24;
    case ma_format_s32:  return AudioFormat::S32;
    case ma_format_f32:  return AudioFormat::F32;
    default:
        return AudioFormat::Unknown;
    }
}

constexpr const char* getAudioFormatString(AudioFormat format)
{
    switch (format) {
    case AudioFormat::U8:   return "U8";
    case AudioFormat::S16:  return "S16";
    case AudioFormat::S24:  return "S24";
    case AudioFormat::S32:  return "S32";
    case AudioFormat::F32:  return "F32";
    default:
        return "Unknown";
    }
}

constexpr size_t getFormatSize(AudioFormat format)
{
    switch (format) {
    case AudioFormat::U8:
        return 1;
    case AudioFormat::S16:
        return 2;
    case AudioFormat::S24:
        return 3;
    case AudioFormat::S32:
        return 4;
    case AudioFormat::F32:
        return 4;
    default:
        return 0;
    }
}

} // namespace AudioUtility

} // namespace ocf::audio
