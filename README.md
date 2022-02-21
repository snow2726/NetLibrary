# NetLibrary
Linux C++ 服务端多线程网络库 (version 0.1.0)
## 项目简介

本项目是基于 Reactor 模式实现的 Linux C++ 服务端多线程网络库，应用层实现了一个 HTTP 服务器HttpServer 和 回显服务器 EchoServer，其中HTTP服务器实现了 HTTP 解析和 Get 方法请求，支持静态资源访问，支持 HTTP 长连接。

## 项目由来

在网络编程的学习过程中，先后阅读了游双老师的《Linux高性能服务器编程》与 陈硕先生的 《Linux多线程服务端编程》，受益匪浅。但是纸上得来终觉浅，绝知此事要躬行，为了加深对网络编程的理解，参照陈硕先生的 muduo 代码，以及网络上优秀的博客与开源项目，使用C++11 一步步实现了一个网络服务器，在此向他们表示由衷的感谢！

## 项目目的

学习C++知识、部分C++11的语法和编码规范、学习巩固网络编程、网络IO模型、IO复用、多线程、git使用、Linux命令、TCP/IP、HTTP协议等知识

## 项目环境

- OS: CentOS Linux release 7.3.1611 (Core)  (# cat /etc/redhat-release)
- kernel: 3.10.0-514.el7.x86_64 (# uname -a)
- Complier: 4.8.5 (# g++ --version)

## 编译与运行

```bash
make 			# 编译可执行文件
./httpserver	          # 运行
make clean 		# 清除运行环境
```

## 项目特点

- 线程模型是多 Reactor 多线程方案，其核心是事件的 one loop per thread 
- 基于对象而非面向对象的设计风格，其事件回调多以 function + bind 表达
- 基于 epoll 的IO复用机制，采用边缘触发（ET）模式
- 采用线程池技术增加并行服务数量，提高CPU利用率
- 支持 HTTP 长连接
- 支持优雅关闭连接
  - 主动关闭连接，先关本地 ”写“ 端，等对方关闭之后，再关本地 ”读“ 端
  - 被动关闭连接，当 read() 返回 0 的时候再进行关闭
  - 如果连接出错，服务端可直接 shutdown

