# customize the following paths for your computer
CC = xc8
LIB_INC_PATH = "./include"

CHIP = 16F1454

CFLAGS = --chip=$(CHIP) -Q -G  --double=24 --float=24
CFLAGS += --rom=default,-0-1FF,-1F7F-1F7F
CFLAGS += --codeoffset=0x200
CFLAGS += --opt=default,+asm,-asmfile,+speed,-space,-debug --addrqual=ignore
CFLAGS += --mode=pro -N64 -I. -I$(LIB_INC_PATH) --warn=0 --asmlist --summary=default,-psect,-class,+mem,-hex,-file --output=default,-inhx032 
CFLAGS += --runtime=default,-clear,+init,-keep,-no_startup,-osccal,-resetbits,-download,-stackcall,+clib

DAPPER_OBJS = usb.p1 usb_hid.p1 usb_descriptors.p1 main.p1 dm.p1

DAPPER_HDRS = usb_config.h dm.h dm_bsp.h swdio_bsp.h

all: dapper.hex

dapper.hex: $(DAPPER_OBJS)
	$(CC) $(CFLAGS) -o./$@ $(DAPPER_OBJS)

%.p1: %.c $(DAPPER_HDRS) Makefile
	$(CC) --pass1 $(CFLAGS) -o./$@ $<

clean:
	rm -f dapper.hex
	rm -f *.p1 *.d *.pre *.sym *.cmf *.cof *.hxl *.lst *.obj *.rlf *.sdb
	rm -f funclist
