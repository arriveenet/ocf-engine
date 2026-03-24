cmake_minimum_required(VERSION 3.20)
project(ocf-engine)

# ==============================================================================
#  Project Settings
# ==============================================================================
set(CXX_STANDARD "-std=c++20")

if (MSVC)
    set(CXX_STANDARD "/std:c++20")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} /W0 /Zc:__cplusplus")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD} -fstrict-aliasing -Wno-unknown-pragmas -Wno-unused-function -Wno-deprecated-declarations")
endif()

# ==============================================================================
#  Sub Projects
# ==============================================================================
add_subdirectory(engine)
