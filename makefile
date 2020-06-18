BUILD_DIR := build
LIB :=  -I ./
CFLAGS := -Wall -m32 -fno-stack-protector $(LIB) -c -fno-builtin -W -std=c++17 -fno-exceptions -fno-threadsafe-statics -nostartfiles -ffreestanding  -fno-rtti   
CC := gcc-7
TARGET := app  
SRC_DIR :=  kernel process thread lib disk 
CPP_SRC := $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.cpp))   
CPP_OBJ := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(CPP_SRC)) 
ASM_SRC := $(filter-out kernel/mbr.asm kernel/load.asm, $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.asm))) 
ASM_OBJ := $(patsubst %.asm, $(BUILD_DIR)/%.asmo, $(ASM_SRC)) 
ALL_OBJ := $(ASM_OBJ) $(CPP_OBJ)

$(TARGET):stop $(ALL_OBJ) BOOT_BIN 
	@ld $(ALL_OBJ) -nostdlib  -Ttext 0xc0001500 -m elf_i386 -e main -o $(BUILD_DIR)/os.bin  
	dd if=$(BUILD_DIR)/os.bin  of=hd60.img count=200 seek=9 conv=notrunc
	@objdump -D  $(BUILD_DIR)/os.bin >  $(BUILD_DIR)/os.dasm 
  
BOOT_BIN: $(BUILD_DIR)/kernel/mbr.asmbin $(BUILD_DIR)/kernel/load.asmbin 
	dd if=$(BUILD_DIR)/kernel/mbr.asmbin  of=hd60.img count=1  conv=notrunc
	dd if=$(BUILD_DIR)/kernel/load.asmbin of=hd60.img seek=2 count=20  conv=notrunc

$(BUILD_DIR)/%.o:%.cpp 
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi;
	$(CC) $(CFLAGS) $< -o $@
 
$(BUILD_DIR)/%.asmo:%.asm 
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi;
	nasm $<  -f elf -o $@

$(BUILD_DIR)/%.asmbin:%.asm
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi;
	nasm $< -o $@ 

run: $(TARGET) 
	@bochs -q >/dev/null 2>&1 &

stop:
	@$(shell pkill bochs)
	@if [ -f  hd60.img.lock ]; then rm hd60.img.lock; fi
	@if [ -f  hd80M.img.lock ]; then rm hd80M.img.lock; fi
  
rebuild: clean  $(TARGET) 

clean:
	rm -rf  $(BUILD_DIR)
