DEFINES=
CC=gcc
LD=gcc
CFLAGS=-O3 -g -std=c99 -W -Wall $(DEFINES)
LDFLAGS=
BINS=sdimage

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

sdimage_obj := sdimage.o
sdimage_obj := $(addprefix $(OBJTREE),$(sdimage_obj))
sdimage: $(sdimage_obj)
	@$(LD) -o $(OBJTREE)/$@ $^ $(LDFLAGS)

clean:
	rm -fr $(OBJTREE)/*.o $(OBJTREE)/$(BINS)

veryclean:
	rm -rf $(BINS)
