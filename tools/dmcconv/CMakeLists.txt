cmake_minimum_required(VERSION 3.20)
project(dmcconv LANGUAGES CXX)

set(TARGET dmcconv)

# ==================================================================================================
#  Includes & target definition
# ==================================================================================================
add_executable(${TARGET} main.cpp)

set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ocf_dmcconv FOLDER Tools)

# miniaudio reads the input files
target_link_libraries(${TARGET} PRIVATE ocfengine miniaudio)

# ==================================================================================================
# Compiler flags
# ==================================================================================================
if (MSVC)
    set(COMPILER_FLAGS /W3 /utf-8)
else()
    set(COMPILER_FLAGS -Wall)
endif()

target_compile_options(${TARGET} PRIVATE ${COMPILER_FLAGS})
