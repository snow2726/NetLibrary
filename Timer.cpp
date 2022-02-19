//
//Timer类，定时器
//对TimerManager类的一层封装
//

#include <sys/time.h>

#include "Timer.h"
#include "TimerManager.h"

//构造函数
Timer::Timer(int timeout, TimerType timertype, const CallBack &timercallback)
	: timeout_(timeout),
	timertype_(timertype),
	timercallback_(timercallback),
	rotation(0),
	timeslot(0),
	prev(nullptr),
	next(nullptr)
{
	if(timeout < 0 )
		return;
}

//析构函数
Timer::~Timer()
{
	Stop();
}

//定时器启动，加入管理器
void Timer::Start()
{
	TimerManager::GetTimerManagerInstance()->AddTimer(this);
}

//定时器撤销，从管理器中删除
void Timer::Stop()
{
	TimerManager::GetTimerManagerInstance()->RemoveTimer(this);
}

 //重新设置定时器
void Timer::Adjust(int timeout, Timer::TimerType timertype, const CallBack &timercallback)
{
	timeout_ = timeout;
	timertype_ = timertype;
	timercallback_ = timercallback;
	TimerManager::GetTimerManagerInstance()->AdjustTimer(this);
}
