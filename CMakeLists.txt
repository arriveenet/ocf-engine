cmake_minimum_required(VERSION 3.20)
project(ocf-engine)

# ==================================================================================================
#  Project Settings
# ==================================================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /W0 /Zc:__cplusplus")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function -Wno-deprecated-declarations")
endif()

# ==================================================================================================
#  Options
# ==================================================================================================
option(OCF_USE_FREETYPE "Use FreeType instead of stb_truetype for TrueType fonts" OFF)

# ==================================================================================================
#  Sub Projects
# ==================================================================================================
add_subdirectory(third-party/cgltf/ocf)

if (OCF_USE_FREETYPE)
    # Disable optional external dependencies of FreeType
    set(FT_DISABLE_ZLIB     ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2    ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG      ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI   ON CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL    ON CACHE BOOL "" FORCE)
    add_subdirectory(third-party/freetype)
    set_target_properties(freetype PROPERTIES FOLDER ThirdParty)
endif()

add_subdirectory(third-party/glfw)
add_subdirectory(third-party/googletest/ocf)
add_subdirectory(third-party/miniaudio/ocf)
add_subdirectory(third-party/stb/ocf)
add_subdirectory(third-party/tinyobjloader)
add_subdirectory(third-party/xxHash)

add_subdirectory(engine)

add_subdirectory(tests)
