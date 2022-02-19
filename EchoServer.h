// 
//EchoServer类，回显服务，把收到的数据发回客户端
//它关注“三个半事件”中的“消息/数据到达”的事件
//

#ifndef _ECHO_SERVER_H_
#define _ECHO_SERVER_H_

#include <string>
#include "TcpServer.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Timer.h"

class EchoServer
{
public:
	typedef std::shared_ptr<TcpConnection> spTcpConnection;
	typedef std::shared_ptr<Timer> spTimer;
	
	//构造函数与析构函数
	EchoServer(EventLoop* loop, const int port, const int threadnum);
	~EchoServer();
	
	//启动服务
	void Start();
	
private:
	//Tcp服务端
	TcpServer tcpserver_;
	
	//业务函数
	void HandleNewConnection(const spTcpConnection& sptcpconn);
	void HandleMessage(const spTcpConnection &sptcpconn, std::string &s);
	void HandleSendComplete(const spTcpConnection &sptcpconn);
	void HandleClose(const spTcpConnection &sptcpconn);
	void HandleError(const spTcpConnection &sptcpconn);

};


#endif
