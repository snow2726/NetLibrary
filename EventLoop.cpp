//
//

#include <iostream>
#include <sys/eventfd.h>
#include <stdlib.h>
#include <unistd.h>

#include "EventLoop.h"

//参照muduo，实现跨线程唤醒
//EventLoop在它的IO线程内执行某个用户任务的回调
//如果用户在当前IO线程调用这个函数，回调会同步进行；如果用户在其他线程调用这个函数，evtfd会被加入队列，IO线程会被唤醒来调用这个Functor
int CreateEventFd()
{
	//eventfd是linux 2.6.22后系统提供的一个轻量级的进程间通信的系统调用
	//eventfd通过一个进程间共享的64位计数器完成进程间通信，这个计数器由在linux内核空间维护
	//如果设置了EFD_CLOEXEC标志，在子进程执行exec的时候，会清除掉父进程的文件描述符
	//EFD_NONBLOCK:非阻塞标志，如果没有设置了这个标志位，那read操作将会阻塞直到计数器中有值
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC );
	if(evtfd < 0)
	{
		std::cout << "Failed in eventfd" << std::endl;
		exit(1);
	}
	return evtfd;
}

EventLoop::EventLoop()		//构造函数中注册回调函数
	:functorlist_(),
	channellist_(),
	activechannellist_(),
	poller_(),
	quit_(true),
	tid_(std::this_thread::get_id()),
	mutex_(),
	wakeupfd_(CreateEventFd()),
	wakeupchannel_()
{
	wakeupchannel_.SetFd(wakeupfd_);		//设置文件描述符
	wakeupchannel_.SetEvents(EPOLLIN | EPOLLET);	//设置触发事件
	wakeupchannel_.SetReadHandle(std::bind(&EventLoop::HandleRead, this));		//注册读事件回调
	wakeupchannel_.SetErrorHandle(std::bind(&EventLoop::HandleError, this));	//注册错误事件回调
	AddChannelToPoller(&wakeupchannel_);		//添加事件
}

EventLoop::~EventLoop()		//析构函数
{
	close(wakeupfd_);
}

//唤醒线程
//由于IO线程平时阻塞在事件循环EventLoop::loop()的epoll调用中，为了让IO线程立刻执行用户回调，需要设法唤醒线程
void EventLoop::WakeUp()
{
	uint64_t one = 1;
	//ssize_t是signed size_t
	//ssize_t 供返回字节计数或错误提示的函数使用
	ssize_t n = write(wakeupfd_, (char*)(&one), sizeof one);	//对wakeupfd_写入数据
}

//读回调
void EventLoop::HandleRead()
{
	uint64_t one = 1;
	ssize_t n = read(wakeupfd_,&one, sizeof one);			//对wakeupfd_读出数据
}

//错误回调
void EventLoop::HandleError()
{
	;
}

//创建了EventLoop对象的线程是IO线程，其主要功能是运行事件循环EventLoop::loop()
//事件循环必须在IO线程执行
void EventLoop::loop()
{
	quit_ = false;
	while(!quit_)	//只要quit_非true，就会一直执行事件循环
	{
		poller_.poll(activechannellist_);	//获取就绪事件列表
		for(Channel *pchannel : activechannellist_)
	{
			pchannel->HandleEvent();	//处理事件
		}
		activechannellist_.clear();
		ExecuteTask();		//执行任务队列的任务
	}
}

