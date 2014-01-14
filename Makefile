CONTRIB_DIR = .
TEST_DIR = ./tests
LLQUEUE_DIR = $(CONTRIB_DIR)/CLinkedListQueue

GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CC     = gcc
CCFLAGS = -g -O2 -Werror -Werror=return-type -Werror=uninitialized -Wcast-align -fno-omit-frame-pointer -fno-common -fsigned-char $(GCOV_CCFLAGS) -I$(LLQUEUE_DIR) -Iinclude

all: tests_main

clinkedlistqueue:
	mkdir -p $(LLQUEUE_DIR)/.git
	git --git-dir=$(LLQUEUE_DIR)/.git init 
	pushd $(LLQUEUE_DIR); git pull git@github.com:willemt/CLinkedListQueue.git; popd

download-contrib: clinkedlistqueue

$(TEST_DIR)/main_test.c:
	if test -d $(LLQUEUE_DIR); \
	then echo have contribs; \
	else make download-contrib; \
	fi
	cd $(TEST_DIR) && sh make-tests.sh "test_*.c" > main_test.c && cd ..

tests_main: raft_server.c raft_log.c raft_node.c $(TEST_DIR)/main_test.c $(TEST_DIR)/test_server.c $(TEST_DIR)/test_node.c $(TEST_DIR)/test_log.c $(TEST_DIR)/test_scenario.c $(TEST_DIR)/mock_send_functions.c $(TEST_DIR)/CuTest.c $(LLQUEUE_DIR)/linked_list_queue.c
	$(CC) $(CCFLAGS) -o $@ $^
	./tests_main

clean:
	rm -f $(TEST_DIR)/main_test.c *.o $(GCOV_OUTPUT)
