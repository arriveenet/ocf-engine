cmake_minimum_required(VERSION 3.20)
project(sandbox LANGUAGES CXX)

set(TARGET sandbox)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(HDRS
    AudioSystemTest.h
    MainScene.h
    MeshInstanceTest.h
    RenderTargetTest.h
    SpriteTest.h
    TestBaseScene.h
)

set(SRCS
    AudioSystemTest.cpp
    MainScene.cpp
    MeshInstanceTest.cpp
    RenderTargetTest.cpp
    TestBaseScene.cpp
    SpriteTest.cpp
    main.cpp
)

# ==================================================================================================
#  Includes & target definition
# ==================================================================================================
add_executable(${TARGET} ${HDRS} ${SRCS})

# The AMD driver disables VSync for an executable named sandbox.exe (a per-application profile)
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ocf_sandbox)

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
