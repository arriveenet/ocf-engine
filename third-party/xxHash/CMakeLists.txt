cmake_minimum_required (VERSION 3.19)
project(xxHash)

set(TARGET xxHash)

add_library(${TARGET} "xxhash.h" "xxhash.c")

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)
