//
//TimerManager类，定时器管理，以时间轮实现
//

#include <iostream>
#include <thread>
#include <ctime>	//此头文件是 C 风格日期和时间库的一部分
#include <chrono>	//在这个头文件下的元素都是处理时间的
#include <ratio>	//提供编译时有理数算术支持
#include <unistd.h>	//对POSIX 操作系统API 的访问功能
#include <sys/time.h>	//Linux系统的日期头文件

#include "TimerManager.h"

//初始化
TimerManager* TimerManager::ptimermanager_ = nullptr;
std::mutex TimerManager::mutex_;
TimerManager::GC TimerManager::gc;			//垃圾回收类成员
const int TimerManager::slotinterval = 1;		//每个slot的时间间隔
const int TimerManager::slotnum = 1024;			//slot总数

//构造函数
TimerManager::TimerManager()
	:currentslot(0),
	timewheel(slotnum,nullptr),
	running_(false),
	th_()
{
	
}

//析构函数
TimerManager::~TimerManager()
{
	Stop();
}

//定时器管理器
TimerManager* TimerManager::GetTimerManagerInstance()
{
	if(ptimermanager_ == nullptr)	//指针为空，则创建管理器
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if(ptimermanager_ == nullptr)	//双重检测
		{
			ptimermanager_ == new TimerManager();
		}
	}
	return ptimermanager_;
}

 //添加定时任务
void TimerManager::AddTimer(Timer* ptimer)
{
	if(ptimer == nullptr)
		return;
	//时间轮互斥量
	std::lock_guard<std::mutex> lock(timewheelmutex_);
	//计算定时器参数
	CalculateTimer(ptimer);
	//添加定时器到时间轮中
	AddTimerToTimeWheel(ptimer);
}

//删除定时任务
void TimerManager::RemoveTimer(Timer* ptimer)
{
	if(ptimer == nullptr)
		return;
	std::lock_guard<std::mutex> lock(timewheelmutex_);
	//从时间轮中移除定时器
	RemoveTimerFromTimeWheel(ptimer);
}

 //调整定时任务
void TimerManager::AdjustTimer(Timer* ptimer)
{
	if(ptimer == nullptr)
		return;
	std::lock_guard<std::mutex> lock(timewheelmutex_);
	//从时间轮中修改定时器
	AdjustTimerToWheel(ptimer);
}

//计算定时器参数
void TimerManager::CalculateTimer(Timer* ptimer)
{
	if(ptimer == nullptr)
		return;
	
	int tick = 0;
	int timeout = ptimer->timeout_;
	if(timeout < slotinterval)
	{
		tick = 1;	//不足一个slot间隔，按延迟1个slot计算
	}
	else
	{
			tick = timeout / slotinterval;	//时间间隔个数 = 超时时间除以时间间隔
		}
		
		ptimer->rotation = tick / slotnum;	//定时器剩下的转数 = tick / slot总数
		int timeslot = (currentslot + tick) % slotnum;	//取模求出定时器所在的时间槽
		ptimer->timeslot = timeslot;		
	}

	//添加定时器到时间轮中
	void TimerManager::AddTimerToTimeWheel(Timer* ptimer)
	{
		if(ptimer == nullptr)
			return;
		
		int timeslot = ptimer->timeslot;
		if(timewheel[timeslot])
		{
			//添加定时器到当前定时器之后
			ptimer->next = timewheel[timeslot];
			timewheel[timeslot]->prev = ptimer;
			timewheel[timeslot] = ptimer;
		}
		else
		{	
			//否则以定时器为首指针
			timewheel[timeslot] = ptimer;
		}
	}

	//从时间轮中移除定时器
	void TimerManager::RemoveTimerFromTimeWheel(Timer* ptimer)
	{
		if(ptimer == nullptr)
			return;
		
		int timeslot = ptimer->timeslot;
		if(ptimer == timewheel[timeslot])
		{
			//头节点
			timewheel[timeslot] = ptimer->next;
			if(ptimer->next != nullptr)
			{
				ptimer->next->prev = nullptr;
			}
			ptimer->prev = ptimer->next = nullptr;
		}
		else
		{
			if(ptimer->prev == nullptr)	//不在时间轮的链表中，即已经被删除了
				return;
			//移除在时间轮链表中的定时器
			ptimer->prev->next = ptimer->next;
			if(ptimer->next != nullptr)
				ptimer->next->prev = ptimer->prev;
			ptimer->prev = ptimer->next = nullptr;
		}
	}

	//从时间轮中修改定时器
	void TimerManager::AdjustTimerToWheel(Timer* ptimer)
	{
		if(ptimer == nullptr)
			return;
		
		//先移除，再计算参数，最后插入定时器
		RemoveTimerFromTimeWheel(ptimer);
		CalculateTimer(ptimer);
		AddTimerToTimeWheel(ptimer);
	}

	//检查超时任务
	void TimerManager::CheckTimer()	//执行当前slot的任务
	{
		std::lock_guard<std::mutex> lock(timewheelmutex_);
		Timer *ptimer = timewheel[currentslot];		//当前slot
		while(ptimer != nullptr)
		{
			if(ptimer->rotation > 0)		//未超时就继续找下一个定时器
			{
				--ptimer->rotation;
				ptimer = ptimer->next;
			}
			else
			{
				//可执行定时器任务，回调函数
				ptimer->timercallback_();	//注意：任务里不能把定时器自身给清理掉！！！我认为应该先移除再执行任务
				if(ptimer->timertype_ == Timer::TimerType::TIMER_ONCE)	//如果是一次触发型
				{
					Timer *ptemptimer = ptimer;
					ptimer = ptimer->next;
					RemoveTimerFromTimeWheel(ptemptimer);		//移出时间轮
				}
				else	// //周期触发型
				{
					Timer *ptemptimer = ptimer;
					ptimer = ptimer->next;
					AdjustTimerToWheel(ptemptimer);			//修改定时器
				if(currentslot == ptemptimer->timeslot && ptemptimer->rotation > 0 )
				{	
					//如果定时器多于一转的话，需要先对rotation减1，否则会等待两个周期
					--ptemptimer->rotation;
				}
			}
		}
	}
	currentslot = (++currentslot) % TimerManager::slotnum;	//移动至下一个时间槽
}

//线程实际执行的函数
void TimerManager::CheckTick()
{
	int si = TimerManager::slotinterval;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int oldtime = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000;
	int time;
	int tickcount;
	while(running_)
	{
		//gettimeofday:获取/设置时间
		gettimeofday(&tv, NULL);
		//tv_sec:自纪元（对于简单的日历时间）或自其他起点（对于经过的时间）以来经过的整秒数
		//tv_usec:自tv_sec成员提供的时间以来经过的微秒数 
		time = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000;
		//计算两次check的时间间隔占多少个slot
		tickcount = (time - oldtime) / slotinterval;	
		oldtime = oldtime + tickcount * slotinterval; 
		
		for(int i=0; i<tickcount; ++i)
		{
			TimerManager::GetTimerManagerInstance()->CheckTimer();
		}
		//milliseconds(si)时间粒度越小，延时越不精确，因为上下文切换耗时
		//阻止当前线程的执行至少指定的sleep_duration，sleep_duration表示睡眠的持续时间
		std::this_thread::sleep_for(std::chrono::microseconds(500));
	}
}

//开启线程，定时器启动
void TimerManager::Start()
{
	running_ = true;
	th_ = std::thread(&TimerManager::CheckTick, this);
}

//暂停定时器
void TimerManager::Stop()
{
	running_ = false;
	if(th_.joinable())	//joinable():检查std::thread对象是否标识活动的执行线程
		th_.join();	//阻塞当前线程，直到被识别的线程 *this 完成其执行
}
