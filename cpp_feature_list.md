# C++语言特性的使用

## 建议使用的特性

兼容C语言的语法特性  
函数重载  
命令空间  
class  
引用

## 谨慎使用

new/delete 使用时，请确保当前内存管理模块已完成初始化，确保不与malloc free混用,此外new/delete无法控制内存是在用户空间还是内核空间，必要时可以重载operator new和operator delete.

## 不建议使用的特性

模板  模板会导致代码膨胀，而本内核需要控制在100kB以内

## 不能使用的特性  

全局对象自动构造 需要运行库以及编译器支持支持，解决办法是手动调用构造函数来构造全局对象  
异常 需要运行库支持  
RTTI(Run-time type Information) 需要运行库支持  
threadsafe statics 需要运行库支持

## 参考资料

[cppreference](https://en.cppreference.com/w/cpp/freestanding)  
[osdev](https://wiki.osdev.org/C++)
