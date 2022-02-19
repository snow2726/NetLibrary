//
//EchoServer类，回显服务，把收到的数据发回客户端
//

#include <iostream>
#include <functional>

#include "EchoServer.h"

//构造函数
EchoServer::EchoServer(EventLoop* loop, const int port, const int threadnum)
		:tcpserver_(loop, port, threadnum)
{	//注册回调函数
	//在函数中使用std::bind()对原函数对象的参数列表进行“包装 ”，来将其适配成我们想要的参数列表形式，需用&符号去传函数指针
	//函数有个隐含的this指针，所以需要this参数进行包装
	//std::placeholders::_1表示占位符，对应新返回的可调用对象的参数列表
	tcpserver_.SetNewConnCallback(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
	tcpserver_.SetMessageCallback(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
	tcpserver_.SetSendCompleteCallback(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
	tcpserver_.SetCloseCallback(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
	tcpserver_.SetErrorCallback(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
}

//析构函数
EchoServer::~EchoServer()
{

}

//启动Tcp服务器
void EchoServer::Start()
{
	tcpserver_.Start();
}

void EchoServer::HandleNewConnection(const spTcpConnection& sptcpconn)	//sptcpconn是TcpConnection对象的shared_ptr
{
	std::cout << "New Connection Come in" << std::endl;
}

//echo服务的业务逻辑
//不必担心Send(msg)是否完整地发送了数据，网络库会帮我们管理发送缓冲区
//体现了“基于事件编程”的典型做法
//即程序主体是被动等待事件发生，事件发生之后网络库会回调事先注册的事件处理函数
void EchoServer::HandleMessage(const spTcpConnection& sptcpconn, std::string& s)
{
	std::string msg;		//发送缓冲区
	msg.swap(s);			//收到的数据放入缓冲区
	msg.insert(0, "reply msg:");
	sptcpconn->Send(msg);		//把收到的数据原封不动地发回客户端
}

void EchoServer::HandleSendComplete(const spTcpConnection& sptcpconn)
{
	
}

void EchoServer::HandleClose(const spTcpConnection& sptcpconn)
{
	std::cout << "Echo Server conn clse" << std::endl;
}

void EchoServer::HandleError(const spTcpConnection& sptcpconn)
{
	std::cout << "Echo Server error" << std::endl;
}
