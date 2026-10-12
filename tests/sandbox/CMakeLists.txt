cmake_minimum_required(VERSION 3.20)
project(sandbox LANGUAGES CXX)

set(TARGET sandbox)

# ==================================================================================================
# Sources and headers
# ==================================================================================================
set(HDRS
)

set(SRCS
    main.cpp
)

# ==================================================================================================
#  Includes & target definition
# ==================================================================================================
add_executable(${TARGET} ${HDRS} ${SRCS})

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
