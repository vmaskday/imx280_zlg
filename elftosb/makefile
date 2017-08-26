#*******************************************************************************
#                               makefile
# Description:
#   gnu make makefile for elftosb executable
 
#*******************************************************************************
#                               Environment

# UNAMES is going to be set to either "Linux" or "CYGWIN_NT-5.1"
UNAMES = $(shell uname -s)

ifdef O
ifeq ("$(origin O)", "command line")
BUILD_DIR := $(O)
endif
endif

ifeq ("${UNAMES}", "Linux")
SRC_DIR = $(shell pwd)
else 
ifeq ("${UNAMES}", "CYGWIN_NT-5.1")
SRC_DIR = $(shell pwd)
endif
endif

#*******************************************************************************
#                                 Targets

all clean elftosb sbtool keygen:
	@mkdir -p ${BUILD_DIR};
	make -C ${BUILD_DIR} -f ${SRC_DIR}/makefile.rules SRC_DIR=${SRC_DIR} $@;
