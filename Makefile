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
	  -D FLATCC_PORTABLE \
	  -I/Users/willem/contrib/flatcc/include \
	  $(GCOV_CCFLAGS) -I$(LLQUEUE_DIR) -Iinclude -g -O2 -fPIC

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
SHAREDFLAGS = -dynamiclib
SHAREDEXT = dylib
# We need to include the El Capitan specific /usr/includes, aargh
CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/
CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk/usr/include
CFLAGS += -fsanitize=address
CFLAGS += -Wno-nullability-completeness
else
SHAREDFLAGS = -shared
SHAREDEXT = so
endif


OBJECTS = src/raft_server.o src/raft_server_properties.o src/raft_node.o src/raft_log.o

TRANSPORT_DIR = include/transport

TRANSPORT_OBJECTS = $(TRANSPORT_DIR)/core_builder.h $(TRANSPORT_DIR)/core_reader.h $(TRANSPORT_DIR)/core_verifier.h $(TRANSPORT_DIR)/flatbuffers_common_builder.h $(TRANSPORT_DIR)/flatbuffers_common_reader.h

all: static shared

.PHONY: transport
transport:
	mkdir -p $(TRANSPORT_DIR)
	/Users/willem/contrib/flatcc/bin/flatcc -a -o $(TRANSPORT_DIR) src/transport/core.fbs

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
shared: transport $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(CFLAGS) -fPIC $(SHAREDFLAGS) -o libraft.$(SHAREDEXT)

.PHONY: static
static: transport $(OBJECTS)
	ar -r libraft.a $(OBJECTS)

.PHONY: tests
tests: src/raft_server.c src/raft_server_properties.c src/raft_log.c src/raft_node.c $(TEST_DIR)/main_test.c $(TEST_DIR)/test_server.c $(TEST_DIR)/test_node.c $(TEST_DIR)/test_log.c $(TEST_DIR)/test_snapshotting.c $(TEST_DIR)/test_scenario.c $(TEST_DIR)/mock_send_functions.c $(TEST_DIR)/CuTest.c $(LLQUEUE_DIR)/linked_list_queue.c
	$(CC) $(CFLAGS) -o tests_main $^
	./tests_main
	gcov raft_server.c

.PHONY: test_fuzzer
test_fuzzer:
	python tests/log_fuzzer.py

.PHONY: tests_full
tests_full:
	make clean
	make tests
	make test_fuzzer
	make test_virtraft

.PHONY: test_virtraft
test_virtraft:
	cp src/*.c virtraft/deps/raft/
	cp include/*.h virtraft/deps/raft/
	cd virtraft; make clean; make; make tests

.PHONY: amalgamation
amalgamation:
	./scripts/amalgamate.sh > raft.h

.PHONY: infer
infer: do_infer

.PHONY: do_infer
do_infer:
	make clean
	infer -- make

clean:
	@rm -f $(TEST_DIR)/main_test.c *.o $(GCOV_OUTPUT); \
	rm $(TRANSPORT_DIR)/*.h; \
	if [ -f "libraft.$(SHAREDEXT)" ]; then rm libraft.$(SHAREDEXT); fi;\
	if [ -f libraft.a ]; then rm libraft.a; fi;\
	if [ -f tests_main ]; then rm tests_main; fi;
