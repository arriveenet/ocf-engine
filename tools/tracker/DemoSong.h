// SPDX-License-Identifier: MIT
#pragma once

#include <ocf/audio/tracker/Song.h>

namespace ocf::tracker_tool {

/** A short song built in code, so that playback can be tried without opening a file. */
audio::tracker::Song createDemoSong();

} // namespace ocf::tracker_tool
