cmake_minimum_required(VERSION 3.0.0)

project("willemt_raft")

include_directories("include")

aux_source_directory(src RAFT_SRC)

file(GLOB TEST_SRC tests/*.c)

message(STATUS ${TEST_SRC})

link_directories(.)

set(${TEST_NAME} "${PROJECT_NAME}_tests_main")

add_library(${PROJECT_NAME} STATIC ${RAFT_SRC})

add_executable("test_main" ${TEST_SRC} ${RAFT_SRC})

