DEFINES=-DCRYPTO_LIBUSB
CC=gcc
LD=gcc
CFLAGS=-O3 -g -std=c99 -W -Wall `pkg-config --cflags libusb-1.0` $(DEFINES)
LDFLAGS=`pkg-config --libs libusb-1.0`
BINS=elftosb sbtoelf sbloader rsrctool elftosb1
TARGET_OUT ?= ../../OUT/tools
${TARGET_OUT}: ${TARGET_OUT}
	mkdir -p $@

elftosb_obj := elftosb.o crc.o crypto.o aes128.o sha1.o elf.o dbparser.o misc.o sb.o


all: $(BINS)

%.o: %.c ${TARGET_OUT}
	$(CC) $(CFLAGS) -c -o ${TARGET_OUT}/$@ $<

sbtoelf: sbtoelf.o crc.o crypto.o aes128.o sha1.o xorcrypt.o dbparser.o elf.o misc.o sb.o sb1.o
	$(LD) -o ${TARGET_OUT}/$@ ${TARGET_OUT}/$^ $(LDFLAGS)

elftosb: elftosb.o crc.o crypto.o aes128.o sha1.o elf.o dbparser.o misc.o sb.o
	cd ${TARGET_OUT} \
	$(LD) -o $@ $^ $(LDFLAGS)

elftosb1: elftosb1.o xorcrypt.o elf.o misc.o sb1.o
	$(LD) -o ${TARGET_OUT}/$@ ${TARGET_OUT}/$^ $(LDFLAGS)

sbloader: sbloader.o
	$(LD) -o ${TARGET_OUT}/$@ ${TARGET_OUT}/$^ $(LDFLAGS)

rsrctool: rsrctool.o rsrc.o misc.o
	$(LD) -o ${TARGET_OUT}/$@ ${TARGET_OUT}/$^ $(LDFLAGS)

clean:
	rm -fr ${TARGET_OUT}/*.o

veryclean:
	rm -rf ${TARGET_OUT}/$(BINS)
