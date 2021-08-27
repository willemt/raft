CONTRIB_DIR = .
TEST_DIR = ./tests
LLQUEUE_DIR = $(CONTRIB_DIR)/CLinkedListQueue
VPATH = src

GCOV_OUTPUT = *.gcda *.gcno *.gcov src/*.gcda src/*.gcno src/*.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
override CFLAGS += -Iinclude -Werror -Werror=return-type -Werror=uninitialized -Wcast-align \
	  -Wno-pointer-sign -fno-omit-frame-pointer -fno-common -fsigned-char \
	  -Wunused-variable \
	  $(GCOV_CCFLAGS) -I$(LLQUEUE_DIR) -Iinclude -g -O2 -fPIC

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
ASANFLAGS = -fsanitize=address
SHAREDFLAGS = -dynamiclib
SHAREDEXT = dylib
# We need to include the El Capitan specific /usr/includes, aargh
override CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/
override CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk/usr/include
override CFLAGS += $(ASANFLAGS)
override CFLAGS += -Wno-nullability-completeness
else
SHAREDFLAGS = -shared
SHAREDEXT = so
endif

OBJECTS = src/raft_server.o src/raft_server_properties.o src/raft_node.o src/raft_log.o

all: static shared

clinkedlistqueue:
	mkdir -p $(LLQUEUE_DIR)/.git
	git --git-dir=$(LLQUEUE_DIR)/.git init 
	pushd $(LLQUEUE_DIR); git pull http://github.com/willemt/CLinkedListQueue; popd

download-contrib: clinkedlistqueue

$(TEST_DIR)/main_test.c: $(TEST_DIR)/test_*.c
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

.PHONY: tests
tests: src/raft_server.c src/raft_server_properties.c src/raft_log.c src/raft_node.c $(TEST_DIR)/main_test.c $(TEST_DIR)/test_*.c $(TEST_DIR)/mock_send_functions.c $(TEST_DIR)/CuTest.c $(LLQUEUE_DIR)/linked_list_queue.c
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
	python3 tests/virtraft2.py --servers 5 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 1 -m 3
	python3 tests/virtraft2.py --servers 7 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 1 -m 3
	python3 tests/virtraft2.py --servers 5 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 2 -m 3
	python3 tests/virtraft2.py --servers 5 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 3 -m 3
	python3 tests/virtraft2.py --servers 5 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 4 -m 3
	python3 tests/virtraft2.py --servers 5 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 5 -m 3
	python3 tests/virtraft2.py --servers 5 -i 20000 --compaction_rate 50 --drop_rate 5 -P 10 --seed 6 -m 3

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
	@rm -f $(TEST_DIR)/main_test.c src/*.o $(GCOV_OUTPUT); \
	if [ -f "libraft.$(SHAREDEXT)" ]; then rm libraft.$(SHAREDEXT); fi;\
	if [ -f libraft.a ]; then rm libraft.a; fi;\
	if [ -f tests_main ]; then rm tests_main; fi;\
	if [ -f tests.c ]; then rm tests.c; fi;\
	if [ -f tests.o ]; then rm tests.o; fi;\
	if [ -f tests.cpython* ]; then rm tests.cpython*; fi;\
	if [ -d CLinkedListQueue ]; then rm -rf CLinkedListQueue; fi;\
	if [ -d .hypothesis ]; then rm -rf .hypothesis; fi;
