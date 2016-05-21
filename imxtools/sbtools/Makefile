DEFINES=-DCRYPTO_LIBUSB
CC=gcc
LD=gcc
CFLAGS=-O3 -g -std=c99 -W -Wall `pkg-config --cflags libusb-1.0` $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`
BINS=elftosb sbtoelf sbloader rsrctool elftosb1

ifdef O
ifeq ("$(origin O)", "command line")
BUILD_DIR := $(O)
endif
endif
ifneq ($(BUILD_DIR),)
# Attempt to create a output directory.
$(shell [ -d ${BUILD_DIR} ] || mkdir -p ${BUILD_DIR})
# Verify if it was successful.
BUILD_DIR := $(shell cd $(BUILD_DIR) && /bin/pwd)
$(if $(BUILD_DIR),,$(error output directory does not exist))
endif # ifneq ($(BUILD_DIR),)
OBJTREE		:= $(if $(BUILD_DIR),$(BUILD_DIR),$(CURDIR))/


all: $(BINS)
	@echo success !

%.o:
	$(CC) $(CFLAGS) -c -o $@ $(notdir $(patsubst %.o,%.c,$@))

elftosb_obj := elftosb.o crc.o crypto.o aes128.o sha1.o elf.o dbparser.o misc.o sb.o
elftosb_obj := $(addprefix $(OBJTREE),$(elftosb_obj))
elftosb: $(elftosb_obj)
	$(CC) -o $(OBJTREE)/$@ $^ $(LDFLAGS)

sbloader_obj := sbloader.o
sbloader_obj := $(addprefix $(OBJTREE),$(sbloader_obj))
sbloader: $(sbloader_obj)
	@$(LD) -o $(OBJTREE)/$@ $^ $(LDFLAGS)

sbtoelf_obj := sbtoelf.o crc.o crypto.o aes128.o sha1.o xorcrypt.o dbparser.o elf.o misc.o sb.o sb1.o
sbtoelf_obj := $(addprefix $(OBJTREE),$(sbtoelf_obj))
sbtoelf: $(sbtoelf_obj)
	@$(LD) -o $(OBJTREE)/$@ $^ $(LDFLAGS)

elftosb1_obj := elftosb1.o xorcrypt.o elf.o misc.o sb1.o
elftosb1_obj := $(addprefix $(OBJTREE),$(elftosb1_obj))
elftosb1: $(elftosb1_obj)
	@$(LD) -o $(OBJTREE)/$@ $^ $(LDFLAGS)

rsrctool_obj := rsrctool.o rsrc.o misc.o
rsrctool_obj :=  $(addprefix $(OBJTREE),$(rsrctool_obj))
rsrctool: $(rsrctool_obj)
	@$(LD) -o $(OBJTREE)/$@ $^ $(LDFLAGS)
clean:
	rm -fr $(OBJTREE)/*.o $(OBJTREE)/$(BINS)

veryclean:
	rm -rf $(BINS)
