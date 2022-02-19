//
//Channel类，fd和事件的封装
//每个Channel对象自始至终只属于一个Eventloop，因此每个Channel对象都只属于一个IO线程
//每个Channel对象只负责一个fd的IO事件分发，Channel会把不同的IO事件分发为不同的回调

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <functional>



class Channel
{
public:
	//回调函数类型
	typedef std::function<void()> Callback;
	
	//构造函数与析构函数
	Channel();
	~Channel();
	
	//设置文件描述符
	void SetFd(int fd)
	{
		fd_=fd;
	}
	
	//获取文件描述符
	int GetFd() const
	{
		return fd_;
	}

	//设置触发事件
	void SetEvents(uint32_t events)
	{
		events_ = events;
	}

	//获取触发事件
	uint32_t GetEvents() const
	{
		return events_;
	}
	
	//事件分发处理，它是Channel的核心，由EventLoop::loop()调用
	//它的功能是根据events_的值分别调用不同的用户回调函数
	void HandleEvent();

	//设置读事件回调
	void SetReadHandle(const Callback &cb)
	{
		readhandler_ = cb;	//提高效率，可以使用move语义，这里暂时还是存在一次拷贝
	}	
	
	//设置写事件回调
	void SetWriteHandle(const Callback &cb)
	{
		writehandler_ = cb;
	}

	//设置错误事件回调
	void SetErrorHandle(const Callback &cb)
	{
		errorhandler_ = cb;
	}

	//设置close事件回调
	void SetCloseHandle(const Callback &cb)
	{
		closehandler_ = cb;
	}

	//Channel的成员函数都只能由IO线程调用，因此更新数据成员都不必加锁
private:
	//文件描述符
	int fd_;
	//事件，一般情况下为epoll events
	uint32_t events_;
	
	//事件触发时执行的回调函数，在TcpConnection类中注册
	//Channle的回调函数会调用TcpConnection的回调函数
	Callback readhandler_;
	Callback writehandler_;
	Callback errorhandler_;
	Callback closehandler_;		//在事件处理期间Channel对象不会被析构

};

#endif
