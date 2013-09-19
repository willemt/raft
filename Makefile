CONTRIB_DIR = ..
HASHMAP_DIR = $(CONTRIB_DIR)/CHashMapViaLinkedList
BITSTREAM_DIR = $(CONTRIB_DIR)/CBitstream
LLQUEUE_DIR = $(CONTRIB_DIR)/CLinkedListQueue

GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CC     = gcc
#CCFLAGS = -g -O2 -Wall -Werror -Werror=return-type -Werror=uninitialized -Wcast-align -fno-omit-frame-pointer -fno-common -fsigned-char $(GCOV_CCFLAGS) -I$(HASHMAP_DIR) -I$(BITSTREAM_DIR) -I$(LLQUEUE_DIR)
CCFLAGS = -g -O2 -Werror -Werror=return-type -Werror=uninitialized -Wcast-align -fno-omit-frame-pointer -fno-common -fsigned-char $(GCOV_CCFLAGS) -I$(HASHMAP_DIR) -I$(BITSTREAM_DIR) -I$(LLQUEUE_DIR)

all: tests_main

chashmap:
	mkdir -p $(HASHMAP_DIR)/.git
	git --git-dir=$(HASHMAP_DIR)/.git init 
	pushd $(HASHMAP_DIR); git pull git@github.com:willemt/CHashMapViaLinkedList.git; popd

cbitstream:
	mkdir -p $(BITSTREAM_DIR)/.git
	git --git-dir=$(BITSTREAM_DIR)/.git init 
	pushd $(BITSTREAM_DIR); git pull git@github.com:willemt/CBitstream.git; popd

clinkedlistqueue:
	mkdir -p $(LLQUEUE_DIR)/.git
	git --git-dir=$(LLQUEUE_DIR)/.git init 
	pushd $(LLQUEUE_DIR); git pull git@github.com:willemt/CLinkedListQueue.git; popd

download-contrib: chashmap cbitstream clinkedlistqueue

main_test.c:
	if test -d $(HASHMAP_DIR); \
	then echo have contribs; \
	else make download-contrib; \
	fi
	sh make-tests.sh "test_*.c" > main_test.c

tests_main: main_test.c raft_server.c raft_candidate.c raft_follower.c raft_leader.c raft_peer.c test_server.c test_server_request_vote.c test_follower.c test_candidate.c test_leader.c test_peer.c mock_send_functions.c CuTest.c $(HASHMAP_DIR)/linked_list_hashmap.c $(LLQUEUE_DIR)/linked_list_queue.c
	$(CC) $(CCFLAGS) -o $@ $^
	./tests_main

clean:
	rm -f main_test.c *.o tests
