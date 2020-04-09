# Common Makefile for including
# Needs the following variables set at a minium:
# NAME :=
# SRC_EXT :=

# Put site overrides (i.e. REPOSITORY_URL, DAOS_STACK_*_LOCAL_REPO) in here
-include Makefile.local

# default to Leap 15 distro for chrootbuild
CHROOT_NAME ?= opensuse-leap-15.1-x86_64
include packaging/Makefile_distro_vars.mk

ifeq ($(DEB_NAME),)
DEB_NAME := $(NAME)
endif

CALLING_MAKEFILE := $(word 1, $(MAKEFILE_LIST))

DOT     := .
RPM_BUILD_OPTIONS += $(EXTERNAL_RPM_BUILD_OPTIONS)

# some defaults the caller can override
BUILD_OS ?= leap.15
PACKAGING_CHECK_DIR ?= ../packaging
LOCAL_REPOS ?= true

COMMON_RPM_ARGS  := --define "%_topdir $$PWD/_topdir" $(BUILD_DEFINES)
SPEC             := $(shell if [ -f $(NAME)-$(DISTRO_BASE).spec ]; then echo $(NAME)-$(DISTRO_BASE).spec; else echo $(NAME).spec; fi)
VERSION           = $(eval VERSION := $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{version}\n' $(SPEC) | sed -n '1p'))$(VERSION)
DEB_VERS         := $(subst rc,~rc,$(VERSION))
DEB_RVERS        := $(subst $(DOT),\$(DOT),$(DEB_VERS))
DEB_BVERS        := $(basename $(subst ~rc,$(DOT)rc,$(DEB_VERS)))
RELEASE           = $(eval RELEASE := $(shell rpm $(COMMON_RPM_ARGS) --specfile --qf '%{release}\n' $(SPEC) | sed -n '$(SED_EXPR)'))$(RELEASE)
SRPM              = _topdir/SRPMS/$(NAME)-$(VERSION)-$(RELEASE)$(DIST).src.rpm
RPMS              = $(eval RPMS := $(addsuffix .rpm,$(addprefix _topdir/RPMS/x86_64/,$(shell rpm --specfile $(SPEC)))))$(RPMS)
DEB_TOP          := _topdir/BUILD
DEB_BUILD        := $(DEB_TOP)/$(NAME)-$(DEB_VERS)
DEB_TARBASE      := $(DEB_TOP)/$(DEB_NAME)_$(DEB_VERS)
SOURCE           ?= $(eval SOURCE := $(shell CHROOT_NAME=$(CHROOT_NAME) $(SPECTOOL) -S -l $(SPEC) | sed -e 2,\$$d -e 's/.*:  *//'))$(SOURCE)
PATCHES          ?= $(eval PATCHES := $(shell CHROOT_NAME=$(CHROOT_NAME) $(SPECTOOL) -l $(SPEC) | sed -e 1d -e 's/.*:  *//' -e 's/.*\///'))$(PATCHES)
SOURCES          := $(addprefix _topdir/SOURCES/,$(notdir $(SOURCE)) $(PATCHES))
ifeq ($(ID_LIKE),debian)
DEBS             := $(addsuffix _$(DEB_VERS)-1_amd64.deb,$(shell sed -n '/-udeb/b; s,^Package:[[:blank:]],$(DEB_TOP)/,p' debian/control))
DEB_PREV_RELEASE := $(shell dpkg-parsechangelog -S version)
DEB_DSC          := $(DEB_NAME)_$(DEB_PREV_RELEASE)$(GIT_INFO).dsc
#Ubuntu Containers do not set a UTF-8 environment by default.
ifndef LANG
export LANG = C.UTF-8
endif
ifndef LC_ALL
export LC_ALL = C.UTF-8
endif
TARGETS := $(DEBS)
else
# CentOS/Suse packages that want a locale set need this.
ifndef LANG
export LANG = en_US.utf8
endif
ifndef LC_ALL
export LC_ALL = en_US.utf8
endif
TARGETS := $(RPMS) $(SRPM)
endif

define distro_map
	    case $(DISTRO_ID) in           \
	        el7) distro="centos7"      \
	        ;;                         \
	        el8) distro="centos8"      \
	        ;;                         \
	        sle12.3) distro="sles12.3" \
	        ;;                         \
	        sl42.3) distro="leap42.3"  \
	        ;;                         \
	        sl15.1) distro="leap15"    \
	        ;;                         \
	    esac;
endef

define install_repos
	for repo in $($(DISTRO_BASE)_PR_REPOS)                              \
	            $(PR_REPOS) $(1); do                                    \
	    branch="master";                                                \
	    build_number="lastSuccessfulBuild";                             \
	    if [[ $$repo = *@* ]]; then                                     \
	        branch="$${repo#*@}";                                       \
	        repo="$${repo%@*}";                                         \
	        if [[ $$branch = *:* ]]; then                               \
	            build_number="$${branch#*:}";                           \
	            branch="$${branch%:*}";                                 \
	        fi;                                                         \
	    fi;                                                             \
	    $(call distro_map)                                              \
	    baseurl=$${JENKINS_URL:-https://build.hpdd.intel.com/}job/daos-stack/job/$$repo/job/$$branch/; \
	    baseurl+=$$build_number/artifact/artifacts/$$distro/;           \
	    $(call install_repo,$$baseurl);                                 \
        done
endef

all: $(TARGETS)

%/:
	mkdir -p $@

%.gz: %
	rm -f $@
	gzip $<

%.bz2: %
	rm -f $@
	bzip2 $<

%.xz: %
	rm -f $@
	xz -z $<

_topdir/SOURCES/%: % | _topdir/SOURCES/
	rm -f $@
	ln $< $@

# At least one spec file, SLURM (sles), has a different version for the
# download file than the version in the spec file.
ifeq ($(DL_VERSION),)
DL_VERSION = $(VERSION)
endif

$(NAME)-$(DL_VERSION).tar.$(SRC_EXT).asc:
	rm -f ./$(NAME)-*.tar.{gz,bz*,xz}.asc
	curl -f -L -O '$(SOURCE).asc'

$(NAME)-$(DL_VERSION).tar.$(SRC_EXT):
	rm -f ./$(NAME)-*.tar.{gz,bz*,xz}
	curl -f -L -O '$(SOURCE)'

v$(DL_VERSION).tar.$(SRC_EXT):
	rm -f ./v*.tar.{gz,bz*,xz}
	curl -f -L -O '$(SOURCE)'

$(DL_VERSION).tar.$(SRC_EXT):
	rm -f ./*.tar.{gz,bz*,xz}
	curl -f -L -O '$(SOURCE)'

$(DEB_TOP)/%: % | $(DEB_TOP)/

$(DEB_BUILD)/%: % | $(DEB_BUILD)/

$(DEB_BUILD).tar.$(SRC_EXT): $(notdir $(SOURCE)) | $(DEB_TOP)/
	ln -f $< $@

$(DEB_TARBASE).orig.tar.$(SRC_EXT) : $(DEB_BUILD).tar.$(SRC_EXT)
	rm -f $(DEB_TOP)/*.orig.tar.*
	ln -f $< $@

deb_detar: $(notdir $(SOURCE)) $(DEB_TARBASE).orig.tar.$(SRC_EXT)
	# Unpack tarball
	rm -rf ./$(DEB_TOP)/.patched ./$(DEB_TOP)/.detar
	rm -rf ./$(DEB_BUILD)/* ./$(DEB_BUILD)/.pc ./$(DEB_BUILD)/.libs
	mkdir -p $(DEB_BUILD)
	tar -C $(DEB_BUILD) --strip-components=1 -xpf $<

# Extract patches for Debian
$(DEB_TOP)/.patched: $(PATCHES) check-env deb_detar | \
	$(DEB_BUILD)/debian/
	mkdir -p ${DEB_BUILD}/debian/patches
	mkdir -p $(DEB_TOP)/patches
	for f in $(PATCHES); do \
          rm -f $(DEB_TOP)/patches/*; \
	  if git mailsplit -o$(DEB_TOP)/patches < "$$f" ;then \
	      fn=$$(basename "$$f"); \
	      for f1 in $(DEB_TOP)/patches/*;do \
	        [ -e "$$f1" ] || continue; \
	        f1n=$$(basename "$$f1"); \
	        echo "$${fn}_$${f1n}" >> $(DEB_BUILD)/debian/patches/series ; \
	        mv "$$f1" $(DEB_BUILD)/debian/patches/$${fn}_$${f1n}; \
	      done; \
	  else \
	    fb=$$(basename "$$f"); \
	    cp "$$f" $(DEB_BUILD)/debian/patches/ ; \
	    echo "$$fb" >> $(DEB_BUILD)/debian/patches/series ; \
	    if ! grep -q "^Description:\|^Subject:" "$$f" ;then \
	      sed -i '1 iSubject: Auto added patch' \
	        "$(DEB_BUILD)/debian/patches/$$fb" ;fi ; \
	    if ! grep -q "^Origin:\|^Author:\|^From:" "$$f" ;then \
	      sed -i '1 iOrigin: other' \
	        "$(DEB_BUILD)/debian/patches/$$fb" ;fi ; \
	  fi ; \
	done
	touch $@


# Move the debian files into the Debian directory.
ifeq ($(ID_LIKE),debian)
$(DEB_TOP)/.deb_files : $(shell find debian -type f) deb_detar | \
	  $(DEB_BUILD)/debian/
	find debian -maxdepth 1 -type f -exec cp '{}' '$(DEB_BUILD)/{}' ';'
	if [ -e debian/source ]; then \
	  cp -r debian/source $(DEB_BUILD)/debian; fi
	if [ -e debian/local ]; then \
	  cp -r debian/local $(DEB_BUILD)/debian; fi
	if [ -e debian/examples ]; then \
	  cp -r debian/examples $(DEB_BUILD)/debian; fi
	if [ -e debian/upstream ]; then \
	  cp -r debian/upstream $(DEB_BUILD)/debian; fi
	if [ -e debian/tests ]; then \
	  cp -r debian/tests $(DEB_BUILD)/debian; fi
	rm -f $(DEB_BUILD)/debian/*.ex $(DEB_BUILD)/debian/*.EX
	rm -f $(DEB_BUILD)/debian/*.orig
ifneq ($(GIT_INFO),)
	cd $(DEB_BUILD); dch --distribution unstable \
	  --newversion $(DEB_PREV_RELEASE)$(GIT_INFO) \
	  "Git commit information"
endif
	touch $@
endif

# see https://stackoverflow.com/questions/2973445/ for why we subst
# the "rpm" for "%" to effectively turn this into a multiple matching
# target pattern rule
$(subst rpm,%,$(RPMS)): $(SPEC) $(SOURCES)
	rpmbuild -bb $(COMMON_RPM_ARGS) $(RPM_BUILD_OPTIONS) $(SPEC)

$(subst deb,%,$(DEBS)): $(DEB_BUILD).tar.$(SRC_EXT) \
	  deb_detar $(DEB_TOP)/.deb_files $(DEB_TOP)/.patched
	rm -f $(DEB_TOP)/*.deb $(DEB_TOP)/*.ddeb $(DEB_TOP)/*.dsc \
	      $(DEB_TOP)/*.dsc $(DEB_TOP)/*.build* $(DEB_TOP)/*.changes \
	      $(DEB_TOP)/*.debian.tar.*
	rm -rf $(DEB_TOP)/*-tmp
	cd $(DEB_BUILD); debuild --no-lintian -b -us -uc
	cd $(DEB_BUILD); debuild -- clean
	git status
	rm -rf $(DEB_TOP)/$(NAME)-tmp
	lfile1=$(shell echo $(DEB_TOP)/$(NAME)[0-9]*_$(DEB_VERS)-1_amd64.deb);\
	  lfile=$$(ls $${lfile1}); \
	  lfile2=$${lfile##*/}; lname=$${lfile2%%_*}; \
	  dpkg-deb -R $${lfile} \
	    $(DEB_TOP)/$(NAME)-tmp; \
	  if [ -e $(DEB_TOP)/$(NAME)-tmp/DEBIAN/symbols ]; then \
	    sed 's/$(DEB_RVERS)-1/$(DEB_BVERS)/' \
	    $(DEB_TOP)/$(NAME)-tmp/DEBIAN/symbols \
	    > $(DEB_BUILD)/debian/$${lname}.symbols; fi
	cd $(DEB_BUILD); debuild -us -uc
	rm $(DEB_BUILD).tar.$(SRC_EXT)
	for f in $(DEB_TOP)/*.deb; do \
	  echo $$f; dpkg -c $$f; done

$(DEB_TOP)/$(DEB_DSC): $(CALLING_MAKEFILE) $(DEB_BUILD).tar.$(SRC_EXT) \
          deb_detar $(DEB_TOP)/.deb_files $(DEB_TOP)/.patched
	rm -f $(DEB_TOP)/*.deb $(DEB_TOP)/*.ddeb $(DEB_TOP)/*.dsc \
	  $(DEB_TOP)/*.dsc $(DEB_TOP)/*.build* $(DEB_TOP)/*.changes \
	  $(DEB_TOP)/*.debian.tar.*
	rm -rf $(DEB_TOP)/*-tmp
	cd $(DEB_BUILD); dpkg-buildpackage -S --no-sign --no-check-builddeps

$(SRPM): $(SPEC) $(SOURCES)
	rpmbuild -bs $(COMMON_RPM_ARGS) $(RPM_BUILD_OPTIONS) $(SPEC)

srpm: $(SRPM)

$(RPMS): $(SRPM) $(CALLING_MAKEFILE)

rpms: $(RPMS)

$(DEBS): $(CALLING_MAKEFILE)

debs: $(DEBS)

ls: $(TARGETS)
	ls -ld $^

# *_LOCAL_* repos are locally built packages.
# *_GROUP_* repos are a local mirror of a group of upstream repos.
# *_GROUP_* repos may not supply a repomd.xml.key.
ifneq ($(REPOSITORY_URL),)
ifneq ($(DAOS_STACK_$(DISTRO_BASE)_LOCAL_REPO),)
$(DISTRO_BASE)_LOCAL_REPOS  := $(REPOSITORY_URL)$(DAOS_STACK_$(DISTRO_BASE)_LOCAL_REPO)/
endif
ifneq ($(DAOS_STACK_$(DISTRO_BASE)_GROUP_REPO),)
$(DISTRO_BASE)_LOCAL_REPOS  += $(REPOSITORY_URL)$(DAOS_STACK_$(DISTRO_BASE)_GROUP_REPO)/
endif
endif

ifeq ($(ID_LIKE),debian)
ifneq ($(DAOS_STACK_REPO_SUPPORT),)
TEST_STR := $(DAOS_STACK_REPO_UBUNTU_$(VERSION_ID_STR)_LIST)
ifneq ($(TEST_STR),)
UBUNTU_REPOS := $(shell curl $(DAOS_STACK_REPO_SUPPORT)$(TEST_STR))
# Additional repos can be added but must be separated by a | character.
UBUNTU_ADD_REPOS = --othermirror "$(UBUNTU_REPOS)"
else
ifneq ($(DAOS_STACK_REPO_UBUNTU_ROLLING_LIST),)
UBUNTU_REPOS := $(shell curl $(DAOS_STACK_REPO_SUPPORT)$(DAOS_STACK_REPO_UBUNTU_ROLLING_LIST))
# Additional repos can be added but must be separated by a | character.
UBUNTU_ADD_REPOS = --othermirror "$(UBUNTU_REPOS)"
endif
endif
# Need to figure out how to support multiple keys, such as for IPMCTL
ifneq ($(DAOS_STACK_REPO_PUB_KEY),)
HAVE_DAOS_STACK_KEY := TRUE

$(DAOS_STACK_REPO_PUB_KEY):
	curl -f -L -O '$(DAOS_STACK_REPO_SUPPORT)$(DAOS_STACK_REPO_PUB_KEY)'
endif
endif

chrootbuild: $(DEB_TOP)/$(DEB_DSC) $(DAOS_STACK_REPO_PUB_KEY)
	sudo pbuilder create \
	    --extrapackages "gnupg ca-certificates" $(DISTRO_ID_OPT)
ifneq ($(HAVE_DAOS_STACK_KEY),)
	printf "apt-key add - <<EOF\n$$(cat $(DAOS_STACK_REPO_PUB_KEY))\nEOF" \
	       | sudo pbuilder --login --save-after-login
endif
	cd $(DEB_TOP); sudo pbuilder --update --override-config $(UBUNTU_ADD_REPOS)
	cd $(DEB_TOP); sudo pbuilder build $(DEB_DSC)
else
chrootbuild: $(SRPM) $(CALLING_MAKEFILE)
	if [ -w /etc/mock/$(CHROOT_NAME).cfg ]; then                                        \
	    echo -e "config_opts['yum.conf'] += \"\"\"\n" >> /etc/mock/$(CHROOT_NAME).cfg;  \
	    $(call distro_map)                                                              \
	    for repo in $($(DISTRO_BASE)_PR_REPOS) $(PR_REPOS); do                          \
	        branch="master";                                                            \
	        build_number="lastSuccessfulBuild";                                         \
	        if [[ $$repo = *@* ]]; then                                                 \
	            branch="$${repo#*@}";                                                   \
	            repo="$${repo%@*}";                                                     \
	            if [[ $$branch = *:* ]]; then                                           \
	                build_number="$${branch#*:}";                                       \
	                branch="$${branch%:*}";                                             \
	            fi;                                                                     \
	        fi;                                                                         \
	        echo -e "[$$repo:$$branch:$$build_number]\n\
name=$$repo:$$branch:$$build_number\n\
baseurl=$${JENKINS_URL:-https://build.hpdd.intel.com/}job/daos-stack/job/$$repo/job/$$branch/$$build_number/artifact/artifacts/$$distro/\n\
enabled=1\n\
gpgcheck=False\n" >> /etc/mock/$(CHROOT_NAME).cfg;                                          \
	    done;                                                                           \
	    if ! $(LOCAL_REPOS); then                                                       \
	        LOCAL_REPOS="";                                                             \
	    else                                                                            \
	        LOCAL_REPOS="$($(DISTRO_BASE)_LOCAL_REPOS)";                                \
	    fi;                                                                             \
	    for repo in $(JOB_REPOS) $$LOCAL_REPOS $($(DISTRO_BASE)_REPOS); do              \
	        repo_name=$${repo##*://};                                                   \
	        repo_name=$${repo_name//\//_};                                              \
	        echo -e "[$${repo_name//@/_}]\n\
name=$${repo_name}\n\
baseurl=$${repo}\n\
enabled=1\n" >> /etc/mock/$(CHROOT_NAME).cfg;                                               \
	    done;                                                                           \
	    echo "\"\"\"" >> /etc/mock/$(CHROOT_NAME).cfg;                                  \
	else                                                                                \
	    echo "Unable to update /etc/mock/$(CHROOT_NAME).cfg.";                          \
            echo "You need to make sure it has the needed repos in it yourself.";           \
	fi
	mock -r $(CHROOT_NAME) $(MOCK_OPTIONS) $(RPM_BUILD_OPTIONS) $<
endif

docker_chrootbuild:
	docker build --build-arg UID=$$(id -u) -t $(BUILD_OS)-chrootbuild \
	             -f packaging/Dockerfile.$(BUILD_OS) .
	docker run --privileged=true -w $$PWD -v=$$PWD:$$PWD              \
	           -it $(BUILD_OS)-chrootbuild bash -c "make chrootbuild"

rpmlint: $(SPEC)
	rpmlint $<

packaging_check:
	if grep -e --repo $(CALLING_MAKEFILE); then                                    \
	    echo "SUSE repos in $(CALLING_MAKEFILE) don't need a \"--repo\" any more"; \
	    exit 2;                                                                    \
	fi
	if ! diff --exclude \*.sw?                              \
	          --exclude debian                              \
	          --exclude .git                                \
	          --exclude Jenkinsfile                         \
	          --exclude libfabric.spec                      \
	          --exclude Makefile                            \
	          --exclude README.md                           \
	          --exclude _topdir                             \
	          --exclude \*.tar.\*                           \
	          --exclude \*.code-workspace                   \
	          --exclude install                             \
	          --exclude packaging                           \
	          -bur $(PACKAGING_CHECK_DIR)/ packaging/; then \
	    exit 1;                                             \
	fi

check-env:
ifndef DEBEMAIL
	$(error DEBEMAIL is undefined)
endif
ifndef DEBFULLNAME
	$(error DEBFULLNAME is undefined)
endif

test:
	@echo "No test defined for this module"

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

show_targets:
	@echo $(TARGETS)

show_makefiles:
	@echo $(MAKEFILE_LIST)

show_calling_makefile:
	@echo $(CALLING_MAKEFILE)

show_git_metadata:
	@echo $(GIT_SHA1):$(GIT_SHORT):$(GIT_NUM_COMMITS)

.PHONY: srpm rpms debs deb_detar ls chrootbuild rpmlint FORCE        \
        show_version show_release show_rpms show_source show_sources \
        show_targets check-env show_git_metadata
