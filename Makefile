CONTRIB_DIR = .
TEST_DIR = ./tests
LLQUEUE_DIR = $(CONTRIB_DIR)/CLinkedListQueue
VPATH = src

GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CFLAGS += -Iinclude -Werror -Werror=return-type -Werror=uninitialized -Wcast-align \
	  -Wno-pointer-sign -fno-omit-frame-pointer -fno-common -fsigned-char \
	  -Wunused-variable \
	  $(GCOV_CCFLAGS) -I$(LLQUEUE_DIR) -Iinclude -g -O2 -fPIC

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
SHAREDFLAGS = -dynamiclib
SHAREDEXT = dylib
# We need to include the El Capitan specific /usr/includes, aargh
CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/
CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk/usr/include
CFLAGS += -fsanitize=address
else
SHAREDFLAGS = -shared
SHAREDEXT = so
endif

OBJECTS = raft_server.o raft_server_properties.o raft_node.o raft_log.o

all: static shared

clinkedlistqueue:
	mkdir -p $(LLQUEUE_DIR)/.git
	git --git-dir=$(LLQUEUE_DIR)/.git init 
	pushd $(LLQUEUE_DIR); git pull http://github.com/willemt/CLinkedListQueue; popd

download-contrib: clinkedlistqueue

$(TEST_DIR)/main_test.c:
	if test -d $(LLQUEUE_DIR); \
	then echo have contribs; \
	else make download-contrib; \
	fi
	cd $(TEST_DIR) && sh make-tests.sh "test_*.c" > main_test.c && cd ..

.PHONY: shared
shared: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(CFLAGS) -fPIC $(SHAREDFLAGS) -o libraft.$(SHAREDEXT)

.PHONY: static
static: $(OBJECTS)
	ar -r libraft.a $(OBJECTS)

tests_main: src/raft_server.c src/raft_server_properties.c src/raft_log.c src/raft_node.c $(TEST_DIR)/main_test.c $(TEST_DIR)/test_server.c $(TEST_DIR)/test_node.c $(TEST_DIR)/test_log.c $(TEST_DIR)/test_snapshotting.c $(TEST_DIR)/test_scenario.c $(TEST_DIR)/mock_send_functions.c $(TEST_DIR)/CuTest.c $(LLQUEUE_DIR)/linked_list_queue.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: tests
tests: tests_main
	./tests_main
	gcov raft_server.c

.PHONY: fuzzer_tests
fuzzer_tests:
	python tests/log_fuzzer.py

.PHONY: amalgamation
amalgamation:
	./scripts/amalgamate.sh > raft.h

.PHONY: infer
infer: do_infer

.PHONY: do_infer
do_infer:
	make clean
	infer -- make static

clean:
	@rm -f $(TEST_DIR)/main_test.c *.o $(GCOV_OUTPUT); \
	if [ -f "libraft.$(SHAREDEXT)" ]; then rm libraft.$(SHAREDEXT); fi;\
	if [ -f libraft.a ]; then rm libraft.a; fi;\
	if [ -f tests_main ]; then rm tests_main; fi;
