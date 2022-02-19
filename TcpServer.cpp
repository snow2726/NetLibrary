//
//TcpServer类,管理accept()获得的TcpConnection
//

#include <iostream>
#include <stdio.h>
#include <thread>
#include <unistd.h>	//对POSIX 操作系统API 的访问功能
#include <fcntl.h>	//文件控制选项
#include <stdlib.h>	//标准库定义
#include <netinet/in.h>	//Internet 地址族
#include <arpa/inet.h>	//Internet 操作
#include <memory>	//内存分配
#include <sys/epoll.h>

#include "TcpServer.h"

//前置声明
void Setnonblocking(int fd);

//构造函数
TcpServer::TcpServer(EventLoop* loop, const int port, const int threadnum)
	:serversocket_(),
	loop_(loop),
	serverchannel_(),
	conncount_(0),
	eventloopthreadpool(loop, threadnum)
{
	serversocket_.SetReuseAddr();		//地址复用
	serversocket_.BindAddress(port);	//绑定地址
	serversocket_.Listen();			//监听地址
	serversocket_.Setnonblocking();		//非阻塞

	//设置socket文件描述符
	serverchannel_.SetFd(serversocket_.fd());
	//设置读事件回调
	serverchannel_.SetReadHandle(std::bind(&TcpServer::OnNewConnection, this));	
	//设置错误事件回调
	serverchannel_.SetErrorHandle(std::bind(&TcpServer::OnConnectionError, this));
}

//析构函数
TcpServer::~TcpServer()
{

}

//启动TCP服务器
void TcpServer::Start()
{
	//执行线程池
	eventloopthreadpool.Start();
	
	//设置事件可读或ET模式
	serverchannel_.SetEvents(EPOLLIN | EPOLLET);
	//添加事件
	loop_->AddChannelToPoller(&serverchannel_);
}

//新Tcp连接，核心功能，业务功能注册，任务分发
//使用Accept()来获得新连接的fd,它保存用户提供的Callback和MessageCallback，在新建TcpConnection时会同样传给后者
void TcpServer::OnNewConnection()
{
	//循环调用accept，获取所有的建立好连接的客户端fd
	struct sockaddr_in clientaddr;
	int clientfd;
	while((clientfd = serversocket_.Accept(clientaddr)) > 0 )
	{
		std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr) 
            			<< ":" << ntohs(clientaddr.sin_port) << std::endl;	//ntohs():将短整型网络字节序数据转为主机字节序数据
		
		if(++conncount_ >= MAXCONNECTION)
		{
			close(clientfd);
			continue;
		}
		Setnonblocking(clientfd);	//设置非阻塞

		//选择IO线程loop，每次从EventLoopThreadPool取得IO Loop
		EventLoop *loop = eventloopthreadpool.GetNextLoop();
		
		//创建连接，让TcpConnection的连接回调由IO Loop线程调用
		std::shared_ptr<TcpConnection> sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
		
		//注册业务函数
		sptcpconnection->SetMessageCallback(messagecallback_);
		sptcpconnection->SetSendCompleteCallback(sendcompletecallback_);
		sptcpconnection->SetCloseCallback(closecallback_);
		sptcpconnection->SetErrorCallback(errorcallback_);

		//断开连接
		sptcpconnection->SetConnectionCleanUp(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
		//当新连接到达时，Accept()会回调OnNewConnection(),后者会创建TcpConnection对象sptcpconnection,把它加入到map
		{
			std::lock_guard<std::mutex> lock(mutex_);
			tcpconnlist_[clientfd] = sptcpconnection;
		}
		
		//TcpServer和TcpConnection的代码都只处理单线程的情况，借助连接回调并引入EventLoopThreadPool让多线程TcpServer的实现易如反掌
		//连接回调，线程的切换发生在连接建立和断开的时刻	
		newconnectioncallback_(sptcpconnection);
		//Bug，应该把事件添加的操作放到最后,否则bug segement fault,导致HandleMessage中的phttpsession==NULL
		//总之就是做好一切准备工作再添加事件到epoll！！！
		sptcpconnection->AddChannelToLoop();
	}
}

//连接清理,bugfix:这里应该由主loop来执行，投递回主线程删除 OR 多线程加锁删除
//该函数把TcpConnection的智能指针对象从ConnectionMap中移除。
//如果用户不持有TcpConnection智能指针的话，智能指针的引用计数已降到1(本身)
void TcpServer::RemoveConnection(std::shared_ptr<TcpConnection> sptcpconnection)
{
	std::lock_guard<std::mutex> lock(mutex_);
	--conncount_;
	tcpconnlist_.erase(sptcpconnection->fd());	//移除fd
}

//异常处理函数
void TcpServer::OnConnectionError()
{
	std::cout << "UNKNOWN EVENT" << std::endl;
	serversocket_.Close();
}

//设置非阻塞
void Setnonblocking(int fd)
{
	int opts = fcntl(fd, F_GETFL);     //fcntl()可对文件描述符做各自控制，F_GETFL用于获取fd的状态标志
	if(opts<0)
        {
                perror("fcntl(fd, GETFL)"); //GETFL:返回（作为函数结果）文件访问模式和文件状态标志
                exit(1);
        }
	 if(fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0)    //F_SETFL:将文件状态标志设置为arg指定的值
        {
                perror("fcntl(fd, SETFL, opts)");
                exit(1);
        }
}
