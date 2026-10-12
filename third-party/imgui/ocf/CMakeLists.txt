cmake_minimum_required(VERSION 3.20)
project(imgui)

set(OUR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Vulkan REQUIRED)

# ==================================================================================================
#  Core
# ==================================================================================================
set(TARGET imgui)

set(PUBLIC_HDR_DIRS
    ${OUR_DIR}
    ${OUR_DIR}/misc/cpp
)

set(PUBLIC_HDRS
    ${OUR_DIR}/imconfig.h
    ${OUR_DIR}/imgui.h
    ${OUR_DIR}/imgui_internal.h
    ${OUR_DIR}/misc/cpp/imgui_stdlib.h
)

set(SRCS
    ${OUR_DIR}/imgui.cpp
    ${OUR_DIR}/imgui_demo.cpp
    ${OUR_DIR}/imgui_draw.cpp
    ${OUR_DIR}/imgui_tables.cpp
    ${OUR_DIR}/imgui_widgets.cpp
    ${OUR_DIR}/misc/cpp/imgui_stdlib.cpp
)

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIRS})
target_compile_definitions(${TARGET} PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)

# ==================================================================================================
#  Backends (GLFW + Vulkan)
# ==================================================================================================
set(TARGET imgui_backend)

set(PUBLIC_HDRS
    ${OUR_DIR}/backends/imgui_impl_glfw.h
    ${OUR_DIR}/backends/imgui_impl_vulkan.h
)

set(SRCS
    ${OUR_DIR}/backends/imgui_impl_glfw.cpp
    ${OUR_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${OUR_DIR}/backends)
target_link_libraries(${TARGET} PUBLIC imgui Vulkan::Vulkan)
target_link_libraries(${TARGET} PRIVATE glfw)
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)
