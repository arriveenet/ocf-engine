cmake_minimum_required(VERSION 3.20)
project(pugixml CXX)

set(TARGET pugixml)

set(PUBLIC_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(PUBLIC_HDRS ../src/pugixml.hpp ../src/pugiconfig.hpp)

set(SRCS ../src/pugixml.cpp)

add_library(${TARGET} STATIC ${PUBLIC_HDRS} ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIR})
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)

install(TARGETS pugixml ARCHIVE DESTINATION lib/${DIST_DIR})
