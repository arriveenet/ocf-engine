cmake_minimum_required (VERSION 3.19)
project(tinyobjloader)

set(TARGET tinyobjloader)

add_library(${TARGET} "tiny_obj_loader.h" "tiny_obj_loader.cc")

target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)
