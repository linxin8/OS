# 内核启动流程 

1. BIOS初始化，加载硬盘第一个扇区（MBR扇区），把计算机交给MBR程序执行。这部分由BIOS自动完成。  
2. MBR把加载load，把计算机交给load程序执行。
3. load先获取系统物理内存大小，打开A20，加载GTE（全局描述符，其中有关于内存段的定义），然后创建PDE(页表目录)，PTE（页表），初始化段，页，把cpu切换保护模式（修改cr0寄存器的标志位）。最后加载内核（按照内核ELF文件加载），把计算机交给内核程序运行（main函数）。
4. 内核按顺序初始化以下模块：  
    + 中断模块
    + 时钟模块
    + 内存模块
    + 线程模块
    + 进程模块
    + 输入输出模块
    + 系统调用模块
    + 硬盘模块
    + 文件系统


### 模块初始化流程
 
#### 中断
 
1. 中断模块首先初始化中断描述符表（包含中断的入口，权限等信息）
2. 创建一个的中断服务程序接受中断，通过中断号来区分中断并调用特定的中断服务程序来处理中断
3. 注册中断服务程序
4. 初始化中断硬件（8259A）
5. 加载中段描述符表

#### 时钟
1. 初始化时钟硬件
2. 注册时钟中断服务

#### 内存
1. 初始化内存数据
2. 把内存分为内核内存区，用户内存区
3. 创建内核内存池，用户内存池 

#### 线程
1. 创建线程列表（就绪线程列表、阻塞线程列表、正在运行线程列表、已结束运行线程列表）
2. 初始化当前线程PCB，创建闲置线程

#### 进程
1. 初始化TSS描述符

#### 输入输出
1. 创建输入缓冲区
2. 注册输入系统中断服务

#### 系统调用
1. 注册系统调用

#### 硬盘
1. 注册硬盘系统中断服务
2. 读取硬盘参数

### 文件系统
1. 读取分区信息
2. 挂载分区
