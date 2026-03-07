cmake_minimum_required (VERSION 3.19)
project(glad)

set(TARGET glad)

add_library(${TARGET} "include/glad/gl.h" "src/gl.c")

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)
