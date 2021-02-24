# 内核开发运行流程
具体代码参考makefile文件
## 编译内核
分别编译汇编代码和c++代码

### 编译汇编代码
指定输出格式为elf格式
```makefile
	nasm $<  -f elf -o $@
```
### 编译c++代码
|参数|意义|解释|
|---|---|---|
|m32|编译为32程序|操作系统是32位的|
|fno-stack-protector|不进行栈保护检查||
|fno-builtin|不是内置函数||
|fno-exceptions|不使用异常||需要标准库支持|
|fno-threadsafe-statics|不使用静态变量线程安全机制|需要标准库支持|
|nostartfiles|不链接系统启动库||
|ffreestanding|不使用标准库|
|fno-rtti|不使用运行时类型识别机制|需要标准库支持|
||||

```makefile

CFLAGS := -Wall -m32 -fno-stack-protector $(LIB) -c -fno-builtin -W -std=c++17 -fno-exceptions -fno-threadsafe-statics -nostartfiles -ffreestanding  -fno-rtti   

``` 
### 链接

链接时，指定程序入口地址为main函数，指定elf格式是386,指定常量区域的首地址是0xc0001500
```makefile
	@ld $(ALL_OBJ) -nostdlib  -Ttext 0xc0001500 -m elf_i386 -e main -o $(BUILD_DIR)/os.bin  

```

### 写入硬盘

将mbr写入硬盘的第1个扇区，将load写入第2-8号扇区。

```makefile 
BOOT_BIN: $(BUILD_DIR)/kernel/mbr.asmbin $(BUILD_DIR)/kernel/load.asmbin 
	dd if=$(BUILD_DIR)/kernel/mbr.asmbin  of=os.img count=1  conv=notrunc
	dd if=$(BUILD_DIR)/kernel/load.asmbin of=os.img seek=2 count=7  conv=notrunc
```
将内核写入硬盘的9-209扇区上
```makefile
	dd if=$(BUILD_DIR)/os.bin  of=os.img count=200 seek=9 conv=notrunc

```
### 运行
运行bochs虚拟机，加载默认的bochsrc配置文件
```makefile 
	@bochs -q >/dev/null 2>&1 &
```
```bochsrc
#第一步，首先设置 Bochs 在运行过程中能够使用的内存，本例为 32MB
megs: 32

#第二步，设置对应真实机器的 BIOS 和 VGA BIOS
romimage: file=/usr/local/share/bochs/BIOS-bochs-latest
vgaromimage: file=/usr/local/share/bochs/VGABIOS-lgpl-latest

#第三步，设置 Bochs 所使用的磁盘，软盘的关键字为 floppy。 
#若只有一个软盘，则使用 floppya 即可，若有多个，则为 floppya，floppyb… 
#floppya: 1_44=a.img, status=inserted


#第四步，选择启动盘符
#boot: floppy #默认从软盘启动，将其注释 
boot: disk  #改为从硬盘启动。我们的任何代码都将直接写在硬盘上，所以不会再有读写软盘的操作

#第五步，设置日志文件的输出
log: bochsout.txt

#第六步，开启或关闭某些功能
#下面是关闭鼠标，并打开键盘
mouse: enabled=0
#keyboard_mapping: enabled=1, map=/usr/share/bochs/keymaps/x11-pc-us.map
keyboard:keymap=/usr/local/share/bochs/keymaps/x11-pc-de.map
# 硬盘设置
ata0-master: type=disk, path="os.img", mode=flat, cylinders=121, heads=16, spt=63
ata0-slave: type=disk, path="disk.img", mode=flat
#开启调试窗口
display_library: x, options="gui_debug"
#指令手动触发断点
magic_break: enabled=1
#gdb not support gdbstub now
#gdbstub: enabled=1, port=1234, text_base=0, data_base=0, bss_base=0
```