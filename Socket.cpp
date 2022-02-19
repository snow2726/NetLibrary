//
//Socket类，对建立Socket连接过程的封装
//

#include <iostream>
#include <stdio.h>
#include <thread>
#include <unistd.h>	//对POSIX 操作系统API 的访问功能
#include <fcntl.h>	//文件控制选项
#include <stdlib.h>	//标准库定义
#include <sys/types.h>	//数据类型
#include <sys/socket.h>	
#include <cstring>
#include <errno.h>	//整数变量错误号

#include "Socket.h"

//构造函数
Socket::Socket()
{
	serverfd_ = socket(AF_INET, SOCK_STREAM, 0);	//创建一个socket，PF_INET代表IPv4协议族，SOCK_STREAM表示TCP协议
	if(serverfd_ = -1)
	{
		perror("socket create fail!");	//perror:打印系统错误消息
		exit(-1);
	}
	std::cout << "server create socket" << serverfd_ << std::endl;
}

//析构函数
Socket::~Socket()
{
	close(serverfd_);
	std::cout << "server close..." << std::endl;
}

//设置Socket选项
void Socket::SetSocketOption()
{
	;
}

//设置地址复用
void Socket::SetReuseAddr()
{
	int on = 1;
	//SOL_SOCKET设置Socket级别，SO_REUSEADDR设置地址复用，以保证socket连接能立即重启
	setsockopt(serverfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

//设置非阻塞IO
void Socket::Setnonblocking()
{
	int opts = fcntl(serverfd_, F_GETFL);     //fcntl()可对文件描述符做各自控制，F_GETFL用于获取fd的状态标志
	if(opts<0)
	{
		perror("fcntl(serverfd_, GETFL)"); //GETFL:返回（作为函数结果）文件访问模式和文件状态标志
		exit(1);
	}
	if(fcntl(serverfd_, F_SETFL, opts | O_NONBLOCK) < 0)	//F_SETFL:将文件状态标志设置为arg指定的值
	{
		perror("fcntl(serverfd_, SETFL, opts)");
		exit(1);
	}
	std::cout << "Server setnonblocking..." << std::endl;
}

//绑定地址端口
bool Socket::BindAddress(int serverport)
{
	struct sockaddr_in serveraddr;	//sockaddr_in：IPv4地址结构体
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;                     	//sin_family主要用来定义是哪种地址族，AF_INET表示IPv4地址族
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);		//htonl:将长整型主机字节序数据转为网络字节序数据,INADDR_ANY表示所有地址
	serveraddr.sin_port = htons(serverport);		//htons:将短整型主机字节序数据转为网络字节序数据
	int resval = bind(serverfd_, (struct sockaddr*)&serveraddr, sizeof(serveraddr));	//指定地址与哪个socket绑定
	if(resval == -1)
	{
		close(serverfd_);
		perror("error bind");
		exit(1);
	}
	std::cout << "server bindaddress..." << std::endl;
	return true;
}

//监听socket
bool Socket::Listen()
{
	if(listen(serverfd_, 8192) < 0)
	{
		perror("error listen");
		close(serverfd_);
		exit(1);
	}
	std::cout << "server listening..." << std::endl;
	return true;
}

//接受socket连接
int Socket::Accept(struct sockaddr_in &clientaddr)
{
	socklen_t lengthofsockaddr = sizeof(clientaddr);
	int clientfd = accept(serverfd_, (struct sockaddr*)&clientaddr, &lengthofsockaddr );   //从listen监听队列接受一个socket连接
	if(clientfd < 0)
	{
		if(errno == EAGAIN)	//EAGAIN:资源暂时不可用
			return 0;
		return clientfd;
	}
	return clientfd;
}

//关闭连接
bool Socket::Close()
{
	close(serverfd_);
	std::cout << "server close..." << std::endl;
	return true;
}
