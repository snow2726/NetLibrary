//
//EventLoopThreadPool类，表示IO线程的event loop pool，线程池的是通用任务线程
//用one loop per thread的思想实现多线程TcpServer的关键步骤是在新建TcpConnection时从event loop pool里挑选一个loop给TcpConnection用
//即多线程TcpServer自己的EventLoop只用来接受新连接，而新连接会用其他EventLoop来执行IO
//

#ifndef _EVENTLOOP_THREAD_POOL_H_
#define _EVENTLOOP_THREAD_POOL_H_

#include <iostream>
#include <string>
#include <vector>

#include "EventLoop.h"
#include "EventLoopThread.h"

class EventLoopThreadPool
{
public:
	//构造函数与析构函数
	EventLoopThreadPool(EventLoop *mainloop, int threadnum=0);
	~EventLoopThreadPool();
	
	//启动线程池
	void Start();
	
	//获取下一个被分发的loop，依据RR轮询策略
	//TcpServer每次新建一个TcpConnection就会调用GetNextLoop()来取得EventLoop
	//如果是单线程服务，每次返回的都是mainloop_，即TcpServer自己用的那个loop
	EventLoop* GetNextLoop();	

private:
	//主loop
	EventLoop *mainloop_;
	
	//线程数量
	int threadnum_;
	
	//线程列表
	std::vector<EventLoopThread*> threadlist_;
	
	//用于轮询分发的索引
	int index_;
	
};

#endif
