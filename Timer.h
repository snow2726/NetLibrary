//
// Timer类，定时器，生命周期由用户自行管理
//


#ifndef _TIMER_H_
#define _TIMER_H_

#include <functional>

class Timer
{
public:
	//定时器任务类型
	typedef std::function<void()> CallBack;
	
	//定时器类型
	typedef enum
	{
		TIMER_ONCE = 0,	//一次触发型
		TIMER_PERIOD	//周期触发型
	} TimerType;
	
	//超时时间,单位ms
	int timeout_;
	
	//定时器类型
	TimerType timertype_;
	
	//回调函数
	CallBack timercallback_;
	
	//定时器剩下的转数
	int rotation;
	
	//定时器所在的时间槽
	int timeslot;
	
	//定时器链表指针
	Timer *prev;
	Timer *next;
	
	//构造函数与析构函数
	Timer(int timeout, TimerType timertype, const CallBack &timercallback);
	~Timer();
	
	//定时器启动，加入管理器
	void Start();
	
	//定时器撤销，从管理器中删除
	void Stop();
	
	//重新设置定时器
	void Adjust(int timeout, Timer::TimerType timertype, const CallBack &timercallback);

private:
	 
};

#endif
