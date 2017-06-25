project(craft)

cmake_minimum_required(VERSION 2.8.11)

add_subdirectory(thirdparty/gtest)

add_library(craft STATIC
    include/raft.h
    include/raft_log.h
    include/raft_private.h
    src/raft_log.c
    src/raft_node.c
    src/raft_server.c
    src/raft_server_properties.c
)
target_include_directories(craft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(linked-list-queue STATIC
    thirdparty/linked-list-queue/linked_list_queue.h
    thirdparty/linked-list-queue/linked_list_queue.c
)
target_include_directories(linked-list-queue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set(LINKED_LIST_QUEUE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/linked-list-queue)

set(GTEST_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/gtest/include)
add_subdirectory(tests)
