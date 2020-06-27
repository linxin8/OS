# os

一个简单的操作系统内核，实现了简单的功能，如启动，线程/进程管理，内存管理，文件管理，输入输出，用户进程，系统调用等，总代码7k行左右。

## 项目环境

bochs  
g++-7  
nasm  
ubuntu  

## 如何运行

1、在ubuntu下安装bochs、g++-7、nasm  
2、下载项目  
3、进入项目目录  
4、执行命令 make run  

## make 参数

make 编译  
make run 编译并运行  
make rebuild 清理项目并重新编译

## 关于C++

[C++语言特性的使用](https://github.com/linxin8/os/blob/master/cpp_feature_list.md)  

## 关于一些技术细节

[一些技术细节](https://github.com/linxin8/os/blob/master/note.md)

## 参考资料

[操作系统真相还原](https://book.douban.com/subject/26745156/)  
[bochs参考手册](http://bochs.sourceforge.net/doc/docbook/user/index.html)  
[osdev](https://wiki.osdev.org)
