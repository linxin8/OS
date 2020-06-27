# 一些技术细节

## 启动

内核起始扇区 0x9  
内核内存位置 0x70000-0x89000  
内核小于100kB

## 内存

分页机制只区分用户级(CPL3)和系统级，系统级可以访问用户级内容，而分段区分4级，不可以跨级访问  
用户物理内存起始地址0x1100000, bitmap 9A1E0,内核物理内存起始地址0x2000000, bitmap 9A000  
用户虚线堆栈：8048000  
i386不支持invlpg指令  

## 文件

一个扇区512B  
一个区块512B

## 段选择子

| 7654321076543 | 2   | 10  |
| ------------- | --- | --- |
| index         | T/I | RPL |

## 分区信息

|      | start lab | sector count | block bltmap | inode bitmap | inode table | block table |
| ---- | --------- | ------------ | ------------ | ------------ | ----------- | ----------- |
| sdb1 | 0x800     | 0x186A1      | 0x802        | 0x81B        | 0x81C       | 0xABC       |
| sdb2 | 0x19000   | 0xEDE0       | 0x19002      | 0x19011      | 0x19012     | 0x192B2     |
  
| Device       | Boot | Start  | End    | Sectors | Size  | Id  | Type  |
| ------------ | ---- | ------ | ------ | ------- | ----- | --- | ----- |
| ./disk.img1 |      | 2048   | 102048 | 100001  | 48.8M | 83  | Linux |
| /disk.img2  |      | 102400 | 163295 | 60896   | 29.8M | 83  | Linux |

## 中断

### CPU自动压栈

#### 跨特权等级

SS  
SP  
EFLAGS  
CS  
IP  
ERROR_CODE (实际有无取决于中断类型)  

#### 非跨特权等级

EFLAGS  
CS  
IP  
ERROR_CODE (实际有无取决于中断类型)  

### 操作系统压栈

#### 统一栈格式

ERROR_CODE (若cpu未压栈，则把0压栈，保持格式统一，易于处理)  

#### 非0x80系统调用中断上下文保护

DS  
ES  
FS  
GS  
EAX  
ECX  
EDX  
EBX  
ESP  
EBP  
ESI  
EDI
中断号

#### 0x80系统调用中断上下文保护

DS  
ES  
FS  
GS  
EAX  
ECX  
EDX  
EBX  
ESP  
EBP  
ESI  
EDI  
EDX  (第三个参数)  
ECX  (第二个参数)  
EBX  (第一个参数)  

### 中断服务

call 中断服务列表[中断服务号]

### 中断返回

参照中断压栈流程出栈
