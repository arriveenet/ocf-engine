cmake_minimum_required(VERSION 3.20)
project(miniaudio)

set(TARGET miniaudio)

set(PUBLIC_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(PUBLIC_HDRS ../miniaudio.h)

set(SRCS ../miniaudio.c)

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)

install(TARGETS miniaudio ARCHIVE DESTINATION lib/${DIST_DIR})
