CONTRIB_DIR = .
TEST_DIR = ./tests
LLQUEUE_DIR = $(CONTRIB_DIR)/CLinkedListQueue
VPATH = src
BUILDDIR ?= .

GCOV_OUTPUT = $(BUILDDIR)/*.gcda $(BUILDDIR)/*.gcno $(BUILDDIR)/*.gcov
GCOV_CCFLAGS ?= -fprofile-arcs -ftest-coverage
SHELL  = /bin/bash
CFLAGS += -Iinclude -Werror -Werror=return-type -Werror=uninitialized -Wcast-align \
	  -Wno-pointer-sign -fno-omit-frame-pointer -fno-common -fsigned-char \
	  -Wunused-variable -Wshadow \
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

OBJECTS = $(BUILDDIR)/raft_server.o $(BUILDDIR)/raft_server_properties.o $(BUILDDIR)/raft_node.o $(BUILDDIR)/raft_log.o

NAME    := raft
SRC_EXT := gz
SOURCE   = v$(VERSION).tar.$(SRC_EXT)
GIT_SHA1        := $(shell git rev-parse HEAD)
GIT_SHA1_SHORT  := $(shell git describe --always --abbrev=7)
GIT_NUM_COMMITS := $(shell git rev-list HEAD --count)

COMMON_RPM_ARGS := --define "%_topdir $$PWD/_topdir"
DIST    := $(shell rpm $(COMMON_RPM_ARGS) --eval %{?dist})
ifeq ($(DIST),)
SED_EXPR := 1p
else
SED_EXPR := 1s/$(DIST)//p
endif
# TODO: Figure out how to restructure so as to not need a raft.spec.in
#VERSION  = $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{version}\n' $(NAME).spec | sed -n '1p')
#RELEASE  = $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{release}\n' $(NAME).spec | sed -n '$(SED_EXPR)')
VERSION := 0.5.0
RELEASE := 1
SRPM    := _topdir/SRPMS/$(NAME)-$(VERSION)-$(RELEASE).git.$(GIT_NUM_COMMITS).$(GIT_SHA1_SHORT)$(DIST).src.rpm
#RPMS     = $(addsuffix .rpm,$(addprefix _topdir/RPMS/x86_64/,$(shell rpm --specfile $(NAME).spec)))
RPMS    := _topdir/RPMS/x86_64/$(NAME)-$(VERSION)-$(RELEASE)$(DIST).x86_64.rpm           \
	   _topdir/RPMS/x86_64/$(NAME)-devel-$(VERSION)-$(RELEASE)$(DIST).x86_64.rpm     \
	   _topdir/RPMS/x86_64/$(NAME)-debuginfo-$(VERSION)-$(RELEASE)$(DIST).x86_64.rpm
SPEC    := $(NAME).spec
SOURCES := $(addprefix _topdir/SOURCES/,$(notdir $(SOURCE)) $(PATCHES))
TARGETS := $(RPMS) $(SRPM)

all: static shared

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: %.c $(wildcard include/*.h) | $(BUILDDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

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
	$(CC) $(OBJECTS) $(LDFLAGS) $(CFLAGS) -fPIC $(SHAREDFLAGS) -o $(BUILDDIR)/libraft.$(SHAREDEXT)

.PHONY: static
static: $(OBJECTS)
	ar -r $(BUILDDIR)/libraft.a $(OBJECTS)

tests_main: src/raft_server.c src/raft_server_properties.c src/raft_log.c src/raft_node.c $(TEST_DIR)/main_test.c $(TEST_DIR)/test_*.c $(TEST_DIR)/mock_send_functions.c $(TEST_DIR)/CuTest.c $(LLQUEUE_DIR)/linked_list_queue.c
	$(CC) $(CFLAGS) -o $(BUILDDIR)/$@ $^

.PHONY: tests
tests: tests_main
	./tests_main
	if [ -n "$(GCOV_CCFLAGS)" ]; then \
	    gcov raft_server.c;           \
	fi

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
	@rm -f $(TEST_DIR)/main_test.c $(BUILDDIR)/*.o $(GCOV_OUTPUT); \
	if [ -f "$(BUILDDIR)/libraft.$(SHAREDEXT)" ]; then rm $(BUILDDIR)/libraft.$(SHAREDEXT); fi;\
	if [ -f $(BUILDDIR)/libraft.a ]; then rm $(BUILDDIR)/libraft.a; fi;\
	if [ -f $(BUILDDIR)/tests_main ]; then rm $(BUILDDIR)/tests_main; fi;

%.$(SRC_EXT): %
	rm -f $@
	gzip $<

$(NAME)-$(VERSION).tar:
	git archive --format tar --prefix $(NAME)-$(VERSION)/ -o $@ HEAD

v$(VERSION).tar: $(NAME)-$(VERSION).tar
	mv $< $@

$(NAME).spec: $(NAME).spec.in Makefile force
	sed -e 's/@PACKAGE_VERSION@/$(VERSION)/g'                   \
	    -e 's/@GIT_SHA1@/$(GIT_SHA1)/g'                         \
	    -e 's/@GIT_SHA1_SHORT@/$(GIT_SHA1_SHORT)/g'             \
	    -e 's/@GIT_NUM_COMMITS@/$(GIT_NUM_COMMITS)/g' < $< > $@

%/:
	mkdir -p $@

_topdir/SOURCES/%: % | _topdir/SOURCES/
	rm -f $@
	ln $< $@

# see https://stackoverflow.com/questions/2973445/ for why we subst
# the "rpm" for "%" to effectively turn this into a multiple matching
# target pattern rule
$(subst rpm,%,$(RPMS)): $(SPEC) $(SOURCES)
	rpmbuild -bb $(COMMON_RPM_ARGS) $(RPM_BUILD_OPTIONS) $(SPEC)

$(SRPM): $(SPEC) $(SOURCES)
	rpmbuild -bs $(COMMON_RPM_ARGS) $(SPEC)

srpm: $(SRPM)

$(RPMS): Makefile

rpms: $(RPMS)

ls: $(TARGETS)
	ls -ld $^

mockbuild: $(SRPM) Makefile
	mock $(MOCK_OPTIONS) $<

rpmlint: $(SPEC)
	rpmlint $<

show_git_metadata:
	@echo $(GIT_SHA1):$(GIT_SHA1_SHORT):$(GIT_NUM_COMMITS)

show_version:
	@echo $(VERSION)

show_release:
	@echo $(RELEASE)

show_rpms:
	@echo $(RPMS)

show_source:
	@echo $(SOURCE)

show_sources:
	@echo $(SOURCES)

.PHONY: srpm rpms ls mockbuild rpmlint FORCE show_version show_release show_rpms show_source show_sources force
