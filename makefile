BUILD_DIR := build
LIB :=  -I ./
CFLAGS := -Wall -m32 -fno-stack-protector $(LIB) -c -fno-builtin -W -std=c++17 -fno-exceptions \
         -fno-threadsafe-statics -nostartfiles -ffreestanding  -fno-rtti 
# CFLAGS = -Wall -m32 -fno-stack-protector $(LIB) -c -fno-builtin -W -std=c++17 -fno-exceptions \
#           -fno-threadsafe-statics -nostdlib -lgcc

OBJS := $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.asmo $(BUILD_DIR)/asm_interface.o $(BUILD_DIR)/interrupt.o \
        $(BUILD_DIR)/interrupt.asmo $(BUILD_DIR)/init.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/memory.o \
		$(BUILD_DIR)/string.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/sync.o \
		$(BUILD_DIR)/thread.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o $(BUILD_DIR)/switch_to.asmo \
		$(BUILD_DIR)/list.o $(BUILD_DIR)/console.o $(BUILD_DIR)/math.o $(BUILD_DIR)/syscall.o \
		$(BUILD_DIR)/stdio.o $(BUILD_DIR)/log.o  $(BUILD_DIR)/new.o  $(BUILD_DIR)/stdlib.o \
		$(BUILD_DIR)/keyboard.o $(BUILD_DIR)/io_queue.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/file_system.o \
		$(BUILD_DIR)/partition.o $(BUILD_DIR)/inode.o $(BUILD_DIR)/directory.o $(BUILD_DIR)/file.o
 
# CRTI_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crti.o)
# CRTI_OBJ:=crti.o
# CRTBEGIN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
# CRTEND_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
# CRTN_OBJ:=$(shell $(CC) $(CFLAGS) -print-file-name=crtn.o)
# CRTN_OBJ:=crtn.o

OBJ_LINK_LIST:=$(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

CC = gcc-7
TARGET = app 

$(TARGET): stop make_bulid_dir $(OBJS) BOOT_BIN
	ld $(OBJS) -nostdlib  -Ttext 0xc0001500 -m elf_i386 -e main -o $(BUILD_DIR)/os.bin 
	# $(CC) -Wall -m32 -e main -Ttext 0xc0001500 -o $(BUILD_DIR)/os.bin $(OBJS) -nostdlib -lgcc
	dd if=$(BUILD_DIR)/os.bin  of=hd60.img count=200 seek=9 conv=notrunc

BOOT_BIN: $(BUILD_DIR)/mbr.asmbin $(BUILD_DIR)/load.asmbin 
	dd if=$(BUILD_DIR)/mbr.asmbin  of=hd60.img count=1  conv=notrunc
	dd if=$(BUILD_DIR)/load.asmbin of=hd60.img seek=2 count=20  conv=notrunc

$(BUILD_DIR)/%.o: kernel/%.cpp kernel/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: lib/%.cpp lib/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: thread/%.cpp thread/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: process/%.cpp process/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: device/%.cpp device/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: file_system/%.cpp file_system/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: disk/%.cpp disk/%.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.asmo: kernel/%.asm
	nasm $<  -f elf -o $@

$(BUILD_DIR)/%.asmbin: kernel/%.asm
	nasm $< -o $@

$(BUILD_DIR)/%.asmo: thread/%.asm
	nasm $<  -f elf -o $@
 
run: $(TARGET) 
	bochs -q >/dev/null 2>&1 &

stop:
	$(shell pkill bochs)
	if [ -f  hd60.img.lock ]; then rm hd60.img.lock; fi
	if [ -f  hd80M.img.lock ]; then rm hd80M.img.lock; fi

make_bulid_dir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

.PHONY : make_bulid_dir

rebuild: clean $(TARGET)

dump_os_disasm:
	objdump -D build/os.bin > os.dasm

clean:
	rm -rf  $(BUILD_DIR)
