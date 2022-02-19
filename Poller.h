//Poller类，对epoller的封装
//Poller是Eventloop的间接成员，只供其owner EventLoop 在IO线程调用，因此无需加锁
//Poller并不拥有Channel，Channel在析构之前必须自己unregister(EventLoop::removeChannel())，避免空悬指针
//Poller进行epoll系统调用，将获取的事件映射到Channel上，由Channel进行事件回调处理

#ifndef _POLLER_H_
#define _POLLER_H_

#include <vector>
#include <map>
#include <mutex>
#include <sys/epoll.h>

#include "Channel.h"


class Poller
{
public:
	//事件指针数组类型
	typedef std::vector<Channel*> ChannelList;
	
	//epoll实例
	int pollfd_;
	
	//events数组，用于传递给epollwait
	std::vector<struct epoll_event> eventlist_;
	
	//事件表,存储从fd到Channel*的映射
	std::map<int,Channel*> channelmap_;
	
	//保护事件map的互斥量
	std::mutex mutex_;
	
public:
	//构造函数与析构函数
	Poller();
	~Poller();
	
	//等待事件，epoll_wait封装
	void poll(ChannelList &activechannellist);
	
	//添加事件，EPOLL_CTL_ADD
	void AddChannel(Channel *pchannel);

	//移除事件，EPOLL_CTL_DEL
	void RemoveChannel(Channel *pchannel);
	
	//修改事件，EPOLL_CTL_MOD
	void UpdateChannel(Channel *pchannel);

private:
	//data
	//int pollfd_;
};


#endif
