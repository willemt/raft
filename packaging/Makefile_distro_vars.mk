# Find out what we are
ID_LIKE := $(shell . /etc/os-release; echo $$ID_LIKE)
# Of course that does not work for SLES-12
ID := $(shell . /etc/os-release; echo $$ID)
VERSION_ID := $(shell . /etc/os-release; echo $$VERSION_ID)
ifeq ($(ID_LIKE),debian)
UBUNTU_VERS := $(shell . /etc/os-release; echo $$VERSION)
ifeq ($(VERSION_ID),19.04)
# Bug - distribution is set to "devel"
DISTRO_ID_OPT = --distribution disco
endif
DISTRO_ID := ubuntu$(VERSION_ID)
DISTRO_BASE = $(basename UBUNTU_$(VERSION_ID))
VERSION_ID_STR := $(subst $(DOT),_,$(VERSION_ID))
endif
ifeq ($(ID),fedora)
# a Fedora-based mock builder
# derive the the values of:
# VERSION_ID (i.e. 7)
# DISTRO_ID (i.e. el7)
# DISTRO_BASE (i.e. EL_7)
# from the CHROOT_NAME
ifeq ($(CHROOT_NAME),epel-7-x86_64)
DIST        := $(shell rpm $(COMMON_RPM_ARGS) --eval %{?dist})
VERSION_ID  := 7
DISTRO_ID   := el7
DISTRO_BASE := EL_7
SED_EXPR    := 1s/$(DIST)//p
endif
ifeq ($(CHROOT_NAME),opensuse-leap-15.1-x86_64)
VERSION_ID  := 15.1
DISTRO_ID   := sl15.1
DISTRO_BASE := LEAP_15
SED_EXPR    := 1p
endif
ifeq ($(CHROOT_NAME),leap-42.3-x86_64)
# TBD if support is ever resurrected
endif
ifeq ($(CHROOT_NAME),sles-12.3-x86_64)
# TBD if support is ever resurrected
endif
endif
ifeq ($(ID),centos)
DISTRO_ID := el$(VERSION_ID)
DISTRO_BASE := $(basename EL_$(VERSION_ID))
DIST        := $(shell rpm $(COMMON_RPM_ARGS) --eval %{?dist})
SED_EXPR    := 1s/$(DIST)//p
define install_repo
	if yum-config-manager --add-repo=$(1); then                  \
	    repo_file=$$(ls -tar /etc/yum.repos.d/*.repo | tail -1); \
	    sed -i -e 1d -e '$$s/^/gpgcheck=False/' $$repo_file;     \
	else                                                         \
	    exit 1;                                                  \
	fi
endef
endif
ifeq ($(findstring opensuse,$(ID)),opensuse)
ID_LIKE := suse
DISTRO_ID := sl$(VERSION_ID)
DISTRO_BASE := $(basename LEAP_$(VERSION_ID))
endif
ifeq ($(ID),sles)
# SLES-12 or 15 detected.
ID_LIKE := suse
DISTRO_ID := sle$(VERSION_ID)
DISTRO_BASE := $(basename SLES_$(VERSION_ID))
endif
ifeq ($(ID_LIKE),suse)
define install_repo
	zypper --non-interactive ar $(1)
endef
endif
ifeq ($(ID_LIKE),debian)
ifndef LANG
export LANG = C.UTF-8
endif
ifndef LC_ALL
export LC_ALL = C.UTF-8
endif
else
ifndef LANG
export LANG = C.utf8
endif
ifndef LC_ALL
export LC_ALL = C.utf8
endif
endif
