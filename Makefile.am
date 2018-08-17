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
check_PROGRAMS += \
	raft-test
raft_test_SOURCES = \
  test/runner.c
raft_test_CFLAGS = $(AM_CFLAGS)
raft_test_CFLAGS += -I$(top_srcdir)/test -DMUNIT_NO_FORK
raft_test_LDADD = libraft.la
raft_test_LDFLAGS =
TESTS = raft-test

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
