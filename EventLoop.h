//
//IO复用流程的抽象，等待事件，处理事件，执行其他任务
//one loop per thread是指每个线程只能有一个EventLoop对象
//EventLoop接受事件，事件到来时，调用Poller类进行系统调用，获取就绪事件的fd，映射到Channel中，再由EventLoop调用Channel进行事件的分发处理

#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_

#include <iostream>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>

#include "Poller.h"
#include "Channel.h"

class EventLoop	//nocopyable
{
public:
	//任务类型
	typedef std::function<void()> Functor;
	
	//事件列表类型
	typedef std::vector<Channel*> ChannelList;
	
	EventLoop();
	~EventLoop();
	
	 //执行事件循环
	 //它调用Poller::poll()获得当前活动事件的Channel列表，然后依次调用每个Channel的handleEvent()函数
	void loop();
	
	//添加事件
	void AddChannelToPoller(Channel *pchannel)
	{
		poller_.AddChannel(pchannel);
	} 

	//移除事件
	void RemoveChannelToPoller(Channel *pchannel)
	{
		poller_.RemoveChannel(pchannel);	//调用Poller::RemoveChannel()
	}
	
	//修改事件
	void UpdateChannelToPoller(Channel *pchannel)
	{
		poller_.UpdateChannel(pchannel);
	}
	
	//退出事件循环
	void Quit()
	{
		quit_=true;
	}
	
	//获取loop所在线程id
	std::thread::id GetThreadId() const
	{
		return tid_;
	}

	//唤醒loop
	void WakeUp();
	
	//唤醒loop后的读回调
	void HandleRead();
	
	//唤醒loop后的错误处理回调
	void HandleError();

	//向任务队列添加任务
	//将functor放入队列，并在必要时唤醒IO线程
	//只有在IO线程的事件回调中调用AddTask()才无需唤醒
	void AddTask(Functor functor)
	{
		{
			std::lock_guard <std::mutex> lock(mutex_);
			functorlist_.push_back(functor);
		}
		WakeUp();	//跨线程唤醒，worker线程唤醒IO线程
	}
	
	//执行任务队列的任务
	void ExecuteTask()
	{
		std::vector<Functor> functorlist;
		{
			std::lock_guard <std::mutex> lock(mutex_);
			//这里把回调列表swap()到局部变量functorlist中，一方面减小了临界区的长度。另一方面避免了死锁
			functorlist.swap(functorlist_);		
		}
		for(Functor &functor : functorlist)
		{
			functor();				//处理任务
		}
		functorlist.clear();
	}
private:
	//任务列表
	std::vector<Functor> functorlist_;
	
	//所有事件,暂时不用
	ChannelList channellist_;
	
	//活跃事件
	ChannelList activechannellist_;
	
	//epoll操作封装
	Poller poller_;
	
	//运行状态
	bool quit_;
	
	//loop所在线程的id
	std::thread::id tid_;
	
	//保护任务列表的互斥量
	std::mutex mutex_;
	
	//跨线程唤醒fd
	int wakeupfd_;
	
	//用于唤醒当前loop的事件
	Channel wakeupchannel_;
	
};


#endif
