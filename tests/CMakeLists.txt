include_directories(
    ${RAFT_INCLUDE_DIRS}
    SYSTEM
    ${GTEST_INCLUDE_DIRS}
    ${LINKED_LIST_QUEUE_INCLUDE_DIRS}
)

add_definitions(-DDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
set(TESTS_DIR ${CMAKE_BINARY_DIR}/bin/tests)
file(MAKE_DIRECTORY ${TESTS_DIR})

set(ALL_TESTS "")

macro(add_unit_test test file)
    add_executable(${test} ${file} mock_send_functions.c)
    target_link_libraries(${test}
        ${ARGN}
        craft
        linked-list-queue
        gtest
        gtest_main
    )

    set_target_properties(${test}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTS_DIR}
        FOLDER "tests"
    )
    add_test(${test} ${TESTS_DIR}/${test})
    list(APPEND ALL_TESTS ${file})
endmacro()

add_unit_test(raft_log_tests test_log.cpp)
add_unit_test(raft_node_tests test_node.cpp)
add_unit_test(raft_scenario_tests test_scenario.cpp)
add_unit_test(raft_server_tests test_server.cpp)
add_unit_test(raft_snapshotting_tests test_snapshotting.cpp)

add_executable(all_tests EXCLUDE_FROM_ALL ${ALL_TESTS} mock_send_functions.c)
target_link_libraries(all_tests
    craft
    linked-list-queue
    gtest
    gtest_main
)

set_target_properties(all_tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${TESTS_DIR}
    FOLDER "tests"
)
