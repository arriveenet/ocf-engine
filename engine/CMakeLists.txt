cmake_minimum_required(VERSION 3.20)
project(ocfengine LANGUAGES CXX)

set(TARGET ocfengine)

# ==================================================================================================
#  Options
# ==================================================================================================
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if (NOT OCF_RHI_BACKEND)
    set(OCF_RHI_BACKEND "Vulkan")
endif()

if (OCF_RHI_BACKEND STREQUAL "Vulkan")
    find_package(Vulkan REQUIRED)
endif()

# ==================================================================================================
#  Functions
# ==================================================================================================
# Adds the files to ENGINE_FILES and puts them in the IDE filter for relativePath (e.g. audio\apu)
function( add_filtered_files relativePath )
    string(REPLACE "/" "\\" filterName ${relativePath})
    source_group(${filterName} FILES ${ARGN})
    set(ENGINE_FILES ${ENGINE_FILES} ${ARGN} PARENT_SCOPE)
endfunction( add_filtered_files )

# Adds all headers and sources in ocf/<relativePath> (not recursive)
function( add_filtered_dir relativePath )
    file(GLOB TMP_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/ocf/${relativePath}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ocf/${relativePath}/*.inl"
        "${CMAKE_CURRENT_SOURCE_DIR}/ocf/${relativePath}/*.cpp")
    add_filtered_files(${relativePath} ${TMP_FILES})
    set(ENGINE_FILES ${ENGINE_FILES} PARENT_SCOPE)
endfunction( add_filtered_dir )

# ==================================================================================================
#  Sources and headers
# ==================================================================================================
add_filtered_dir("audio")
add_filtered_dir("audio/apu")
add_filtered_dir("audio/tracker")
add_filtered_dir("core")
add_filtered_dir("core/event")
add_filtered_dir("core/input")
add_filtered_dir("core/job")
add_filtered_dir("math")
add_filtered_dir("platform")
add_filtered_dir("renderer")
add_filtered_dir("resource")
add_filtered_dir("rhi")
add_filtered_dir("scene")
add_filtered_dir("scene/2d")
add_filtered_dir("scene/3d")
add_filtered_dir("scene/animation")

if (OCF_RHI_BACKEND STREQUAL "Vulkan")
    add_filtered_dir("rhi/vulkan")
    add_filtered_dir("rhi/vulkan/resource")

    if (WIN32)
        add_filtered_files("rhi/vulkan/platform"
            ocf/rhi/vulkan/platform/VulkanPlatformWindows.h
            ocf/rhi/vulkan/platform/VulkanPlatformWindows.cpp
        )
    elseif (LINUX)
        add_filtered_files("rhi/vulkan/platform"
            ocf/rhi/vulkan/platform/VulkanPlatformLinux.h
            ocf/rhi/vulkan/platform/VulkanPlatformLinux.cpp
        )
    endif()
elseif (OCF_RHI_BACKEND STREQUAL "D3D12")
    add_filtered_dir("rhi/d3d12")
endif()

# ==================================================================================================
#  Includes & target definition
# ==================================================================================================
#building a library
add_library(${TARGET} ${ENGINE_FILES})
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${TARGET} PRIVATE OCF_LIBRARY_EXPORTS)
else()
    target_compile_definitions(${TARGET} PUBLIC OCF_LIBRARY_STATIC)
endif()

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(${TARGET} PROPERTIES FOLDER Engine)

# ==================================================================================================
# Dependencies
# ==================================================================================================
target_link_libraries(${TARGET} PRIVATE cgltf)
target_link_libraries(${TARGET} PRIVATE glfw)
target_link_libraries(${TARGET} PRIVATE miniaudio)
target_link_libraries(${TARGET} PRIVATE pugixml)
target_link_libraries(${TARGET} PRIVATE stb)
target_link_libraries(${TARGET} PRIVATE tinyobjloader)
target_link_libraries(${TARGET} PRIVATE xxHash)
target_link_libraries(${TARGET} PRIVATE yyjson)

# The backend in use is 1 and the others are 0, so that they can be tested with #if
if (OCF_RHI_BACKEND STREQUAL "Vulkan")
    target_link_libraries(${TARGET} PRIVATE Vulkan::Vulkan)
    target_compile_definitions(${TARGET} PUBLIC OCF_RHI_VULKAN=1 OCF_RHI_D3D12=0)
elseif (OCF_RHI_BACKEND STREQUAL "D3D12")
    target_link_libraries(${TARGET} PRIVATE d3d12 dxgi dxguid)
    target_compile_definitions(${TARGET} PUBLIC OCF_RHI_VULKAN=0 OCF_RHI_D3D12=1)
endif()

# Always linked, so that games can use ImGui without checking OCF_ENABLE_IMGUI
target_link_libraries(${TARGET} PUBLIC imgui)
if (OCF_ENABLE_IMGUI)
    target_link_libraries(${TARGET} PRIVATE imgui_backend)
    target_compile_definitions(${TARGET} PUBLIC OCF_ENABLE_IMGUI=1)
else()
    target_compile_definitions(${TARGET} PUBLIC OCF_ENABLE_IMGUI=0)
endif()

# ==================================================================================================
#  Compiler flags
# ==================================================================================================
if (MSVC)
    set(OCF_WARNINGS /W4)
    target_compile_options(${TARGET} PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(${TARGET} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
else()
    set(OCF_WARNINGS
            -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
            -Wextra-semi -Wdeprecated -Wundef
            -Wnon-virtual-dtor -Wimplicit-fallthrough
            -Wno-template-id-cdtor
    )
endif()

target_compile_options(${TARGET} PRIVATE ${OCF_WARNINGS})

# ==================================================================================================
# Sub-projects
# ==================================================================================================
add_subdirectory(test)

