//
//ThreadPool类，简易线程池实现，表示worker线程,执行通用任务线程
//计算线程，负责业务计算任务
//
//使用的同步原语有 
// pthread_mutex_t mutex_l;				//互斥锁
// pthread_cond_t condtion_l;				//条件变量
//
// 使用的系统调用有
// pthread_mutex_init();				//初始化互斥锁
// pthread_cond_init();					//初始化条件变量
// pthread_create(&thread_[i],NULL,threadFunc,this)	//创建一个新线程
// pthread_mutex_lock()					//上锁
// pthread_mutex_unlock()				//解锁
// pthread_cond_signal()				//解锁被阻挡在指定的条件变量，该线程中的至少一个COND
// pthread_cond_wait()					//用于阻塞当前线程，等待别的线程使用 pthread_cond_signal() 或 pthread_cond_broadcast来唤醒它
// pthread_cond_broadcast();				//解除目前被阻塞上指定的条件变量的所有线程COND
// pthread_join()					//加入终止的线程
// pthread_mutex_destory()				//销毁互斥锁
// pthread_cond_destory()				//销毁信号量


#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>

class ThreadPool
{
public:
	//线程池任务类型
	typedef std::function<void()> Task;	//std::function是一个函数包装器模板,该函数包装器模板能包装任何类型的可调用元素
	
	//构造函数与析构函数
	ThreadPool(int threadnum = 0);
	~ThreadPool();		
	
	//启动线程池
	void Start();
	
	//暂停线程池
	void Stop();
	
	//添加任务
	void AddTask(Task task);
	
	//线程池执行的函数
	void ThreadFunc();
	
	//获取线程数量
	int GetThreadNum()
	{
		return threadnum_;
	}
	
private:
	//运行状态
	bool started_;
	
	//线程数量
	int threadnum_;
	
	//线程列表
	std::vector<std::thread*> threadlist_;
	
	//任务队列
	std::queue<Task> taskqueue_;
	
	//任务队列互斥量
	std::mutex mutex_;
	
	//任务队列同步的条件变量
	std::condition_variable condition_;

};

#endif
