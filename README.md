# os

一个简单的操作系统内核，实现了简单的功能，如启动，线程/进程管理，内存管理，文件管理，输入输出，用户进程，系统调用等，总代码7k行左右。


## Demo
```c++
#include "disk/file_system.h"
#include "kernel/asm_interface.h"
#include "kernel/init.h"
#include "kernel/interrupt.h"
#include "kernel/keyboard.h"
#include "kernel/memory.h"
#include "kernel/timer.h"
#include "lib/debug.h"
#include "lib/macro.h"
#include "lib/new.h"
#include "lib/stdint.h"
#include "lib/stdio.h"
#include "lib/stdlib.h"
#include "process/process.h"
#include "thread/sync.h"
#include "thread/thread.h"

int main()
{
    init_all();
    printf("main pid %d\n", getpid());
    Process::execute((void*)user_main, "u1");
    Interrupt::enable();
    while (true)
    {
        Thread::yield();
    }
    return 0;
}

void user_main(void* arg)
{
    UNUSED(arg);
    printf("\nuser process pid %d\n\n", getpid());
    int32_t test = 0;
    while (true)
    {
        char    buffer[2];
        int32_t fd[2];
        if (pipe(fd) == -1)
        {
            printf("pip failed");
            while (true) {}
        }
        auto pid = fork();
        if (pid == 0)
        {
            test--;
            printf("i am child %d, test addr: %x, value %d\n\n", getpid(), &test, test);
            write(fd[1], "123456", 6);
            printf("write ok\n");
            while (true) {}
        }
        test++;
        printf("i am parent %d, child %d, test addr: %x, value %d\n\n", getpid(), pid, &test, test);
        while (true)
        {
            read(fd[0], buffer, 1);
            printf("%c", buffer[0]);
        }
        while (true) {}
    }
} 
```
![GitHub](https://github.com/linxin8/OS/demo.png "demo")

## 项目环境

bochs  
g++-10  
nasm  
ubuntu  

## 环境搭建

### 安装nasm
```
sudo apt install nasm
```

### 安装bochs

```sh
sudo apt install -y g++ libx11-dev libxrandr-dev  build-essential libgtk2.0-dev libreadline-dev libcanberra-gtk-module
wget https://sourceforge.net/projects/bochs/files/bochs/2.6.9/bochs-2.6.9.tar.gz/download
mv download bochs.tar
tar -xvf bochs.tar bochs-2.6.9/
cd bochs-2.6.9/
./configure --enable-debugger --with-x11 --enable-all-optimizations
make
sudo make install
```


## 运行
 
1. 下载项目  
2. 进入项目目录  
3. 执行命令 make run  

## make 参数

make 编译  
make run 编译并运行  
make rebuild 清理项目并重新编译

## 内核开发与启动流程

[开发流程](https://github.com/linxin8/os/blob/master/development.md)

[启动流程](https://github.com/linxin8/os/blob/master/boot.md)
 
## 关于C++

[C++语言特性的使用](https://github.com/linxin8/os/blob/master/cpp_feature_list.md)  

## 关于一些技术细节

[一些技术细节](https://github.com/linxin8/os/blob/master/note.md)

## 参考资料

[操作系统真相还原](https://book.douban.com/subject/26745156/)  
[bochs参考手册](http://bochs.sourceforge.net/doc/docbook/user/index.html)  
[osdev](https://wiki.osdev.org)
