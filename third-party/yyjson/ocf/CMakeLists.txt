cmake_minimum_required(VERSION 3.20)
project(yyjson C)

set(TARGET yyjson)

set(PUBLIC_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(PUBLIC_HDRS ../src/yyjson.h)

set(SRCS ../src/yyjson.c)

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)

install(TARGETS yyjson ARCHIVE DESTINATION lib/${DIST_DIR})
