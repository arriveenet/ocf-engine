cmake_minimum_required(VERSION 3.20)
project(ocf-engine)

# ==================================================================================================
#  Project Settings
# ==================================================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================================================================================================
#  Options
# ==================================================================================================
# When OFF, ImGui is still compiled so that games build unchanged, but it is never initialized
option(OCF_ENABLE_IMGUI "Enable Dear ImGui debug UI" ON)

# ==================================================================================================
#  Compiler Settings
# ==================================================================================================
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /W0 /Zc:__cplusplus")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function -Wno-deprecated-declarations")
endif()

# ==================================================================================================
#  Sub Projects
# ==================================================================================================
add_subdirectory(third-party/cgltf/ocf)
add_subdirectory(third-party/glfw)
add_subdirectory(third-party/googletest/ocf)
add_subdirectory(third-party/imgui/ocf)
add_subdirectory(third-party/miniaudio/ocf)
add_subdirectory(third-party/pugixml/ocf)
add_subdirectory(third-party/stb/ocf)
add_subdirectory(third-party/tinyobjloader)
add_subdirectory(third-party/xxHash)
add_subdirectory(third-party/yyjson/ocf)

add_subdirectory(engine)

add_subdirectory(tools)

add_subdirectory(tests)
