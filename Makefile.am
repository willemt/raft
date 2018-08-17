ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign subdir-objects

lib_LTLIBRARIES =
check_PROGRAMS =
EXTRA_DIST =
noinst_HEADERS =
include_HEADERS =

EXTRA_DIST += raft.pc.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = raft.pc

AM_CFLAGS = -I$(top_srcdir)/include -Wall
if DEBUG
  AM_CFLAGS += -O0 -g -fprofile-arcs -ftest-coverage -DRAFT_DEBUG
else
  AM_CFLAGS += -O2
endif
if EXPERIMENTAL
  AM_CFLAGS += -DRAFT_EXPERIMENTAL
endif

lib_LTLIBRARIES += libraft.la
libraft_la_LDFLAGS = -version-info 0:7:0
libraft_la_SOURCES = \
  src/raft_log.c \
  src/raft_node.c \
  src/raft_server.c \
  src/raft_server_properties.c
include_HEADERS += \
  include/raft.h \
  include/raft_log.h \
  include/raft_private.h \
  include/raft_types.h

# Tests
clinkedlistqueue:
	mkdir -p CLinkedListQueue/.git
	git --git-dir=CLinkedListQueue/.git init 
	pushd CLinkedListQueue; git pull http://github.com/willemt/CLinkedListQueue; popd

.PHONY: CLinkedListQueue/linked_list_queue.c
CLinkedListQueue/linked_list_queue.c:
	if ! test -d CLinkedListQueue/.git; \
	then make clinkedlistqueue; \
	fi

.PHONY: tests/main_test.c
tests/main_test.c:
	cd tests && sh make-tests.sh "test_*.c" > main_test.c && cd ..

check_PROGRAMS += \
  raft-test \
  test-unit

raft_test_SOURCES = \
  CLinkedListQueue/linked_list_queue.c \
  tests/main_test.c \
  tests/CuTest.c \
  tests/CuTest.h \
  tests/mock_send_functions.c \
  tests/mock_send_functions.h \
  tests/test_log.c \
  tests/test_node.c \
  tests/test_scenario.c \
  tests/test_server.c \
  tests/test_snapshotting.c
raft_test_CFLAGS = $(AM_CFLAGS)
raft_test_CFLAGS += -I$(top_srcdir)/tests -I$(top_srcdir)/CLinkedListQueue
raft_test_LDADD = libraft.la
raft_test_LDFLAGS =

test_lib_SOURCES = \
  test/lib/munit.c \
  test/lib/store.c

test_unit_SOURCES = $(test_lib_SOURCES)
test_unit_SOURCES += \
  test/unit/main.c
test_unit_CFLAGS = $(AM_CFLAGS)
test_unit_CFLAGS += -I$(top_srcdir)/tests/unit
test_unit_LDADD = libraft.la
test_unit_LDFLAGS =

TESTS = raft-test test-unit

cov-reset:
if DEBUG
	@lcov --directory src --zerocounters
endif

cov-clean:
	@rm -fr coverage
	@find . -name "*.gcda" -o -name '*.gcno' -delete

cov-report:
	@mkdir -p coverage
	@lcov --compat-libtool --directory src --capture --output-file coverage/raft.info
	@genhtml -o coverage/ coverage/raft.info

clean-local:
	@make cov-clean

check: cov-reset
if DEBUG
	@make cov-report
endif
