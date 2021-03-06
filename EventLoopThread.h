//
//IO线程不一定是主线程，我们可以在任何一个线程创建并运行EventLoop
//一个程序也可以有不止一个IO线程，一次定义EventLoopThread类用于创建IO线程
//EventLoopThread类，表示IO线程,执行特定任务的,线程池的是通用任务的IO线程
//

#ifndef _EVENTLOOP_THREAD_H_
#define _EVENTLOOP_THREAD_H_

#include <iostream>
#include <thread>
#include <string>

#include "EventLoop.h"

class EventLoopThread
{
public:
	//构造函数与析构函数
	EventLoopThread();
	~EventLoopThread();	
	
	//获取当前线程运行的loop
	EventLoop* GetLoop();
	
	//启动线程
	void Start();
	
	//线程执行的函数
	void ThreadFunc();
private:
	//线程成员
	std::thread th_;
	
	//线程id
	std::thread::id threadid_;
	
	//线程名字
	std::string threadname_;
	
	//线程运行的loop循环
	EventLoop *loop_;

};

#endif
