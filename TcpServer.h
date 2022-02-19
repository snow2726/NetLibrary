//
// TcpServer类的功能是管理Socket类获得的TcpConnection，TcpServer会为新连接创建对应的TcpConnection对象
// TcpServer是供用户使用的，生命期由用户控制
//

#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <functional>
#include <string>
#include <map>
#include <mutex>

#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "TcpConnection.h"

#define MAXCONNECTION 20000	//最大连接数

class TcpServer
{
public:
	//TcpConnection智能指针类型
	typedef std::shared_ptr<TcpConnection> spTcpConnection;
	
	//回调函数类型
	//用户只需要设置好callback，再调用start()即可
	typedef std::function<void(const spTcpConnection&, std::string&)> MessageCallback;
	typedef std::function<void(const spTcpConnection&)> Callback;
	
	//构造函数与析构函数
	TcpServer(EventLoop* loop, const int port, const int threadnum = 0);
	~TcpServer();

	//启动TCP服务器
	void Start();
	
	//业务函数注册
	//注册新连接回调函数
	void SetNewConnCallback(const Callback &cb)
	{
		newconnectioncallback_ = cb;
	}

	//注册数据回调函数
	void SetMessageCallback(const MessageCallback &cb)
	{
		messagecallback_ = cb;
	}

	//注册数据发送完成回调函数
	void SetSendCompleteCallback(const Callback &cb)
	{
		sendcompletecallback_ = cb;
	}

	//注册连接关闭回调函数
	void SetCloseCallback(const Callback &cb)
	{
		closecallback_ = cb;
	}

	//注册连接异常h回调函数
	void SetErrorCallback(const Callback &cb)
	{
		errorcallback_ = cb;
	}
private:
	//服务器Socket
	Socket serversocket_;
	
	//主loop
	EventLoop *loop_;
	
	//服务器事件
	Channel serverchannel_;
	
	//连接数量统计
	int conncount_;
	
	//Tcp连接表
	//TcpServer持有目前存活的TcpConnection的shared_ptr，因为TcpConnection对象的生命期是模糊的，用户也可以持有TcpConnection
	//每个TcpConnection对象有一个名字，这个名字由其所属的TcpServer在创建TcpConnection对象时生成
	//名字是ConnectMap的key
	std::map<int, std::shared_ptr<TcpConnection>> tcpconnlist_;	
	
	//保护Tcp连接表的互斥量
	std::mutex mutex_;
	
	//IO线程池
	EventLoopThreadPool eventloopthreadpool;

	//业务接口回调函数
	Callback 	newconnectioncallback_;	//new connection事件
	MessageCallback messagecallback_;	//数据事件
	Callback 	sendcompletecallback_;	//send事件
	//TcpServer向TcpConnection注册CloseCallback，用于接收连接断开的消息
	//通常TcpServer的生命期长于它建立的TcpConnection，因此不用担心TcpServer对象失效
	Callback	closecallback_;		//close事件
	Callback	errorcallback_;		//错误事件
	
	//服务器对新连接连接处理的函数
	void OnNewConnection();
	
	//移除Tcp连接的函数
	void RemoveConnection(const std::shared_ptr<TcpConnection> sptcpconnection);
	
	//服务器Socket的异常处理函数
	void OnConnectionError();
};

#endif
