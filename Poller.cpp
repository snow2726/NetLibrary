//
//
//Poller类，读epoll调用的封装
//

#include <iostream>

#include <stdio.h>	//perror
#include <stdlib.h>	//exit

#include <unistd.h>	//close

#include <errno.h>

#include "Poller.h"

#define EVENTNUM 4096 	//最大触发事件数量
#define TIMEOUT 1000	//epoll_wait 超时时间设置

Poller::Poller()	//构造函数
	: pollfd_(-1),
	eventlist_(EVENTNUM),
	channelmap_(),
	mutex_()
{
	pollfd_ = epoll_create(256);	//创建一个epoll句柄(fd)
	if(pollfd_ == -1)
	{
		perror("epoll_create1");	//perror:打印系统错误消息
		exit(1);
	}
	std::cout << "epoll_create" << pollfd_ << std::endl;
}

Poller::~Poller()	//析构函数
{
	close(pollfd_);
}

//等待I/O事件,是Poller类的核心功能
void Poller::poll(ChannelList &activechannellist)
{
	int timeout = TIMEOUT;
	//调用epoll获得当前活动的IO事件，然后填充调用方传入的activechannellist，并返回就绪的fd
	int nfds = epoll_wait(pollfd_, &*eventlist_.begin(), (int)eventlist_.capacity(), timeout);
	if(nfds == -1)
	{
		perror("epoll wait error");
	}
	//遍历所有nfds，把它对应的Channel填入activechannellist
	for(int i=0; i<nfds; ++i)
	{
		int events = eventlist_[i].events;
		Channel *pchannel = (Channel*)eventlist_[i].data.ptr;
		int fd = pchannel->GetFd();
		
		std::map<int, Channel*>::const_iterator iter;
		{	//为防止在遍历channelmap_时有线程来改变epollfd的大小，进行加锁
			std::lock_guard <std::mutex> lock(mutex_);
			iter = channelmap_.find(fd);
		}
		if(iter != channelmap_.end())
		{
			pchannel->SetEvents(events);
			activechannellist.push_back(pchannel);
		}
		else 
		{
			std::cout << "not find channel!" << std::endl;
		}
	}
	//如果nfds等于eventlist容量，重置eventlist的大小
	if(nfds == (int)eventlist_.capacity())
	{
		std::cout << "resize:" << nfds <<std::endl;
		eventlist_.resize(nfds*2);
	}
}

//添加事件
void Poller::AddChannel(Channel *pchannel)
{
	int fd = pchannel->GetFd();
	struct epoll_event ev;
	ev.events = pchannel->GetEvents();
	//data是联合体
	ev.data.ptr = pchannel;
	{	//加锁
		std::lock_guard<std::mutex> lock(mutex_);
		channelmap_[fd] = pchannel;
	}
	
	if(epoll_ctl(pollfd_, EPOLL_CTL_ADD, fd, &ev) == -1)
	{
		perror("epoll add error");
		exit(1);
	}
}

//删除事件
void Poller::RemoveChannel(Channel *pchannel)
{
	int fd = pchannel->GetFd();
	struct epoll_event ev;
	ev.events = pchannel->GetEvents();
	ev.data.ptr = pchannel;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		channelmap_.erase(fd);
	}

	if(epoll_ctl(pollfd_, EPOLL_CTL_DEL, fd, &ev) == -1)
	{
		perror("epoll del error");
		exit(1);
	}
}

//更新事件
void Poller::UpdateChannel(Channel *pchannel)
{
	int fd = pchannel->GetFd();
    	struct epoll_event ev;
    	ev.events = pchannel->GetEvents();
	ev.data.ptr = pchannel;
	
	if(epoll_ctl(pollfd_, EPOLL_CTL_MOD, fd, &ev) == -1)
	{
		perror("epoll update error");
        	exit(1);
	}
}
	
