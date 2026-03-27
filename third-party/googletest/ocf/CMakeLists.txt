cmake_minimum_required(VERSION 3.19)
project(gtest)

set(OUR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(TARGET gtest)
set(TEST_SRC_DIR ${OUR_DIR}/googletest/src)
set(MOCK_SRC_DIR ${OUR_DIR}/googlemock/src)
set(PUBLIC_HDR_DIRS
    ${OUR_DIR}/googletest/include
    ${OUR_DIR}/googlemock/include
)
set(PRIVATE_HDR_DIRS
    ${OUR_DIR}/googletest
    ${OUR_DIR}/googlemock
)

set(SRCS
    ${TEST_SRC_DIR}/gtest-all.cc
    ${MOCK_SRC_DIR}/gmock-all.cc
)

include_directories(
    ${PUBLIC_HDR_DIRS}
    ${PRIVATE_HDR_DIRS}
)

add_library(${TARGET} STATIC ${SRCS})

target_include_directories(${TARGET} PUBLIC ${PUBLIC_HDR_DIRS})
set_target_properties(${TARGET} PROPERTIES FOLDER ThirdParty)
