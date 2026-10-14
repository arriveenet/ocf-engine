cmake_minimum_required(VERSION 3.20)
project(tracker LANGUAGES CXX)

set(TARGET tracker)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(HDRS
    DemoSong.h
    DpcmEditor.h
    EditorState.h
    FileDialog.h
    FrameEditor.h
    InstrumentEditor.h
    PatternEditor.h
    TrackerAudio.h
    TrackerScene.h
)

set(SRCS
    DemoSong.cpp
    DpcmEditor.cpp
    FileDialog.cpp
    FrameEditor.cpp
    InstrumentEditor.cpp
    main.cpp
    PatternEditor.cpp
    TrackerScene.cpp
)

# ==================================================================================================
#  Includes & target definition
# ==================================================================================================
add_executable(${TARGET} ${HDRS} ${SRCS})

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ocf_tracker FOLDER Tools)

target_link_libraries(${TARGET} PRIVATE ocfengine)

# ==================================================================================================
# Compiler flags
# ==================================================================================================
if (MSVC)
    set(COMPILER_FLAGS /W3 /utf-8)
else()
    set(COMPILER_FLAGS -Wall)
endif()

target_compile_options(${TARGET} PRIVATE ${COMPILER_FLAGS})
