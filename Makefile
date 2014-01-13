CONTRIB_DIR = ..
LLQUEUE_DIR = $(CONTRIB_DIR)/CLinkedListQueue

GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CC     = gcc
CCFLAGS = -g -O2 -Werror -Werror=return-type -Werror=uninitialized -Wcast-align -fno-omit-frame-pointer -fno-common -fsigned-char $(GCOV_CCFLAGS) -I$(LLQUEUE_DIR)

all: tests_main

clinkedlistqueue:
	mkdir -p $(LLQUEUE_DIR)/.git
	git --git-dir=$(LLQUEUE_DIR)/.git init 
	pushd $(LLQUEUE_DIR); git pull git@github.com:willemt/CLinkedListQueue.git; popd

download-contrib: clinkedlistqueue

main_test.c:
	if test -d $(LLQUEUE_DIR); \
	then echo have contribs; \
	else make download-contrib; \
	fi
	sh make-tests.sh "test_*.c" > main_test.c

tests_main: main_test.c raft_server.c raft_log.c raft_node.c test_server.c test_server_request_vote.c test_node.c test_log.c test_scenario.c mock_send_functions.c CuTest.c $(LLQUEUE_DIR)/linked_list_queue.c
	$(CC) $(CCFLAGS) -o $@ $^
	./tests_main

clean:
	rm -f main_test.c *.o $(GCOV_OUTPUT)
