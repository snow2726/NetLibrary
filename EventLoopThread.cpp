//
//

#include <iostream>
#include <sstream>

#include "EventLoopThread.h"

//构造函数
EventLoopThread::EventLoopThread()
	:th_(),
	threadid_(-1),
	threadname_("IO thread"),
	loop_(NULL)
{
	
}

//析构函数
EventLoopThread::~EventLoopThread()
{
	//线程结束时清理
	std::cout << "Clean up the EventLoopThread id:" << std::this_thread::get_id() << std::endl;
	loop_->Quit();	//停止IO线程运行
	th_.join();	//清理IO线程，防止内存泄露，因为pthread_created回calloc
}

EventLoop* EventLoopThread::GetLoop()
{
	return loop_;
}

void EventLoopThread::Start()
{
	//创建线程
	th_ = std::thread(&EventLoopThread::ThreadFunc, this);
}

void EventLoopThread::ThreadFunc()
{	
	//在栈上定义EventLoop对象，将其地址赋值给loop_成员变量
	EventLoop loop;
	loop_ = &loop;
	
	threadid_ = std::this_thread::get_id();	//get_id()用于获取线程id
	std::stringstream sin;
	sin << threadid_;
	threadname_ += sin.str();	//str()将其转为字符串
	
	std::cout << "in the thread:" << threadname_ << std::endl;
	try
	{
		loop_->loop();	//启动线程，运行EventLoop::loop()
	}
	catch(std::bad_alloc& ba)
	{										//what():获取字符串识别异常
		std::cerr << "bad_alloc caught in EventLoopThread::ThreadFunc loop:" << ba.what() << '\n';
	}
}
