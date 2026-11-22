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

# The graphics API the engine renders with. Only one backend is built.
set(OCF_RHI_BACKEND "Vulkan" CACHE STRING "Graphics API backend of the RHI (Vulkan or D3D12)")
set_property(CACHE OCF_RHI_BACKEND PROPERTY STRINGS Vulkan D3D12)

if (NOT OCF_RHI_BACKEND MATCHES "^(Vulkan|D3D12)$")
    message(FATAL_ERROR "OCF_RHI_BACKEND must be Vulkan or D3D12, not '${OCF_RHI_BACKEND}'")
endif()
if (OCF_RHI_BACKEND STREQUAL "D3D12" AND NOT WIN32)
    message(FATAL_ERROR "The D3D12 backend is only available on Windows")
endif()
message(STATUS "OCF RHI backend: ${OCF_RHI_BACKEND}")

# TODO: Remove when ImGuiLayer supports Direct3D 12
if (OCF_RHI_BACKEND STREQUAL "D3D12" AND OCF_ENABLE_IMGUI)
    message(WARNING "Dear ImGui is not supported by the D3D12 backend yet: OCF_ENABLE_IMGUI is turned off")
    set(OCF_ENABLE_IMGUI OFF)
endif()

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
