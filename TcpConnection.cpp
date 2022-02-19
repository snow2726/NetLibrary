//
//TcpConnection类，表示客户端连接
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>		//对POSIX 操作系统API 的访问功能
#include <sys/types.h>          //数据类型
#include <sys/socket.h>
#include <errno.h>

#include "TcpConnection.h"

#define BUFSIZE 4096 		//缓冲区大小


//前置声明
int recvn(int fd, std::string &bufferin);
int sendn(int fd, std::string &bufferout);

//构造函数
//TcpConnection使用Channel来获得socket上的IO事件，它会自己处理可写事件，而把可读事件通过MessageCallback传达给客户
//TcpConnection表示的是“一次TCP连接”，它是不可再生的，一旦连接断开，这个TcpConnetion对象就没用了
//fd参数是已经建立连接的sockfd
TcpConnection::TcpConnection(EventLoop *loop, int fd, const struct sockaddr_in &clientaddr)
	: loop_(loop),
	spchannel_(new Channel()),
	fd_(fd),
	clientaddr_(clientaddr),
	halfclose_(false),
	disconnected_(false),
	asyncprocessing_(false),
	bufferin_(),
	bufferout_()
{	
	//注册事件执行函数
	spchannel_->SetFd(fd_);
	spchannel_->SetEvents(EPOLLIN | EPOLLET);
	spchannel_->SetReadHandle(std::bind(&TcpConnection::HandleRead, this));
	spchannel_->SetWriteHandle(std::bind(&TcpConnection::HandleWrite, this));
	spchannel_->SetCloseHandle(std::bind(&TcpConnection::HandleClose, this));
	spchannel_->SetErrorHandle(std::bind(&TcpConnection::HandleError, this));
}

//析构函数
//TcpConnection拥有TCP socket,它的析构函数会close(fd)(在Socket的析构函数中发生) 
TcpConnection::~TcpConnection()
{
	//多线程下，加入loop的任务队列？不用，因为已经在当前loop线程
	//移除事件,析构成员变量
	loop_->RemoveChannelToPoller(spchannel_.get());
	close(fd_);
}

//添加本连接对应的事件到loop
void TcpConnection::AddChannelToLoop()
{
	loop_->AddTask(std::bind(&EventLoop::AddChannelToPoller, loop_, spchannel_.get()));
}

//发送数据的函数
void TcpConnection::Send(const std::string &s)
{
	bufferout_ += s;	//跨线程消息投递成功
	//判断当前线程是不是Loop IO线程
	if(loop_->GetThreadId() == std::this_thread::get_id())
	{	//是IO线程，直接调用SendInLoop()
		SendInLoop();
	}
	else	//如果在非IO线程调用，它会把message复制一份，传给IO线程中的SendInLoop()来发送
	{
		asyncprocessing_ = false;	//异步调用结束
		loop_->AddTask(std::bind(&TcpConnection::SendInLoop, shared_from_this())); //跨线程调用,加入IO线程的任务队列，唤醒
	}
}

//在当前IO线程发送数据函数
void TcpConnection::SendInLoop()
{
	if(disconnected_)
	{
		return;
	}
	int result = sendn(fd_, bufferout_);	//使用缓冲区发送数据
	if(result > 0)
	{
		uint32_t events = spchannel_->GetEvents();
		if(bufferout_.size() > 0)	//如果数据没发完，有剩余数据留在缓冲区
		{
			//缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
			spchannel_->SetEvents(events | EPOLLOUT);
			//高水位回调，缓冲区已满时触发
			loop_->UpdateChannelToPoller(spchannel_.get());
		}
		else
		{
			//数据已发完
			spchannel_->SetEvents(events & (~EPOLLOUT));
			//低水位回调，发送缓冲区被清空时调用
			sendcompletecallback_(shared_from_this());
			if(halfclose_)	//处于半关闭状态(对方关闭连接)
			{
				HandleClose();	//进行关闭处理
			}
		}
	}
	else if(result < 0)
	{
		HandleError();
	}
	else 
	{
		HandleClose();
	}
}

//主动清理连接
//Shutdown()是线程安全的，它会把实际工作放到ShutdownInLoop()中来做，后者保证在IO线程调用
void TcpConnection::Shutdown()
{
	if(loop_->GetThreadId() == std::this_thread::get_id())
	{	//是IO线程，直接调用ShutdownInLoop()
		ShutdownInLoop();
	}
	else
	{
		//不是IO线程，则是跨线程调用,加入IO线程的任务队列，唤醒，进行回调
		loop_->AddTask(std::bind(&TcpConnection::ShutdownInLoop, shared_from_this()));
	}
}

//在当前IO线程清理连接函数
void TcpConnection::ShutdownInLoop()
{
	if(disconnected_) 
	{
		return;
	}
	std::cout << "shutdown" << std::endl;
	closecallback_(shared_from_this());	//应用层清理连接回调
	loop_->AddTask(std::bind(connectioncleanup_, shared_from_this()));	//自己不能清理自己，交给loop执行，Tcpserver清理TcpConnection
	disconnected_ = true;
}

//读回调函数
//检查recvn函数的返回值，依据返回值分别调用messagecallback_、handleclose()、handleError()
void TcpConnection::HandleRead()
{
	//接收数据，写入缓冲区
	int result = recvn(fd_, bufferin_);	//调用recvn()
	
	//业务回调,可以利用工作线程池处理，投递任务
	if(result>0)
	{
		messagecallback_(shared_from_this(), bufferin_);	//使用Buffer来读取数据，可以用右值引用优化
	}
	else if(result == 0)
	{
		HandleClose();
	}
	else
	{
		HandleError();
	}
}

//写回调函数
void TcpConnection::HandleWrite()
{
	int result = sendn(fd_, bufferout_);	//调用sendn()去发送缓冲区的数据
	if(result > 0)
	{
		uint32_t events = spchannel_->GetEvents();
		if(bufferout_.size() > 0)
		{
			//缓冲区满了，数据没发完，就设置EPOLLOUT事件触发
			spchannel_->SetEvents(events | EPOLLOUT);
			//高水位回调，缓冲区已满时触发
			loop_->UpdateChannelToPoller(spchannel_.get());
		}
		else
		{
			//数据已发完
			spchannel_->SetEvents(events & (~EPOLLOUT));
			//低水位回调，发送缓冲区被清空就调用
			sendcompletecallback_(shared_from_this());
			//发送完毕，如果是半关闭状态，则可以close了
			if(halfclose_)
				HandleClose();
		}
	}
	else if(result < 0)
	{
		HandleError();
	}
	else
	{
		HandleClose();
	}
}

//错误事件回调
void TcpConnection::HandleError()
{
	if(disconnected_) { return; }
	errorcallback_(shared_from_this());
	loop_->AddTask(std::bind(connectioncleanup_, shared_from_this()));	 //自己不能清理自己，交给loop执行，Tcpserver清理
	disconnected_ = true;	
}

//对端关闭连接,有两种，一种close，另一种是shutdown(半关闭)，但服务器并不清楚是哪一种，只能按照最保险的方式来，即发完数据再close
void TcpConnection::HandleClose()
{
	//优雅关闭，发完数据再关闭
	if(disconnected_) { return; }
	if(bufferout_.size() > 0 || bufferin_.size() > 0 || asyncprocessing_)
	{
		//如果还有数据待发送，则先发完,设置半关闭标志位
		halfclose_ = true;
		//还有数据刚刚才收到，但同时又收到FIN
		if(bufferin_.size() > 0)
		{
			messagecallback_(shared_from_this(), bufferin_);
		}
	}
	else	//如果数据已经发完
	{
		loop_->AddTask(std::bind(connectioncleanup_, shared_from_this()));
		closecallback_(shared_from_this()); //调用closecallback_，这个回调绑定到TcpServer::RemoveConnection()
		disconnected_ = true;
	}
}

//带缓冲区的读函数
int recvn(int fd, std::string &bufferin)
{
	int nbyte = 0;
	int readsum = 0;
	char buffer[BUFSIZE];
	for(;;)
	{
		nbyte = read(fd, buffer, BUFSIZE);
		
		if(nbyte > 0)
		{
			bufferin.append(buffer, nbyte);	//效率较低，2次拷贝
			readsum += nbyte;
			if(nbyte < BUFSIZE)
				return readsum;	//读优化，减小一次读调用，因为一次调用耗时10+us
			else 
				continue;
		}
		else if(nbyte < 0)	//异常
		{	//反复调用read()，直到其返回EAGAIN
			if(errno == EAGAIN) //系统缓冲区未有数据，非阻塞返回
			{
				return readsum;
			}
			else if(errno == EINTR)
			{
				std::cout << "errno == EINTR" << std::endl;
				continue; 
			}
			else 
			{
				//可能是RST
				perror("recv error");
				return -1;
			}	
		}
		else	//返回0，客户端关闭socket，FIN
		{
			return 0;
		}
	}
}

//带缓冲区的写函数，可跨线程调用
int sendn(int fd, std::string &bufferout)
{
	ssize_t nbyte = 0;
	int sendsum = 0;
	size_t length = 0;
	
	//无拷贝优化
	length = bufferout.size();
	if(length >= BUFSIZE)
	{
		length = BUFSIZE;
	}
	for(;;)
	{
		nbyte = write(fd, bufferout.c_str(), length);
		if(nbyte < 0)
		{
			sendsum += nbyte;
			bufferout.erase(0, nbyte);
			length = bufferout.size();	
			if(length >= BUFSIZE)
			{
				length = BUFSIZE;
			}
			if(length == 0)
			{
				return sendsum;
			}
		}
		else if(nbyte < 0)	//异常
		{	////反复调用write()，直到其返回EAGAIN
			if(errno == EAGAIN)	//系统缓冲区满，非阻塞返回
			{
				std::cout << "write errno == EAGAIN,not finish!" << std::endl;
				return sendsum;
			}
			//当某些系统调用被中断并且系统无法在中断处理后恢复系统调用时，errno 被设置为 EINTR。开发者可以通过检查 errno 来选择重试系统调用
			else if(errno == EINTR)
			{
				std::cout <<  "write errno == EINTR" << std::endl;
				continue;
			}
			else if(errno == EPIPE)
			{
				//客户端已经close，并发了RST，继续wirte会报EPIPE，返回0，表示close
				perror("write error");	//RST：传输控制协议中的复位标志
				std::cout << "write errno == client send RST" << std::endl;
				return -1;
			}
			else
			{
				perror("write error");
				std::cout << "write error, unknow error" << std::endl;
				return -1;
			}
		}
		else	//返回0
		{	
			//应该不会返回0
			return 0;
		}
	}
}
