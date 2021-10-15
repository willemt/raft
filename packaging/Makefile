NAME    := libfabric
SRC_EXT := gz
SOURCE   = https://github.com/ofiwg/$(NAME)/archive/v$(VERSION).tar.$(SRC_EXT)

OSUSE_HPC_REPO := https://download.opensuse.org/repositories/science:/HPC
LEAP_42_HPC_REPO := $(OSUSE_HPC_REPO)/openSUSE_Leap_42.3/

ifeq ($(DAOS_STACK_LEAP_42_GROUP_REPO),)
LEAP_42_REPOS  := $(LEAP_42_HPC_REPO)
endif
ifeq ($(DAOS_STACK_SLES_12_GROUP_REPO),)
SLES_12_REPOS := $(LEAP_42_HPC_REPO)
endif
ifeq ($(DAOS_STACK_LEAP_15_GROUP_REPO),)
LEAP_15_REPOS := $(OSUSE_HPC_REPO)/openSUSE_Leap_15.1/
endif

include Makefile_packaging.mk
