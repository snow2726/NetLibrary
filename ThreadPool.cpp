//
//ThreadPool类，简易线程池实现，表示worker线程,执行通用任务线程
//计算线程
//

#include <unistd.h>
#include <deque>

#include "ThreadPool.h"

//构造函数
ThreadPool::ThreadPool(int threadnum)
	: started_(false),
	threadnum_(threadnum),
	threadlist_(),
	taskqueue_(),
	mutex_(),
	condition_()
{

}

//析构函数
ThreadPool::~ThreadPool()
{
	std::cout << "Clean up the ThreadPool" << std::endl;
	Stop();	//暂停线程池
	for(int i = 0; i < threadnum_; ++i)
	{
		threadlist_[i]->join();		//终止线程
	}
	for(int i = 0; i < threadnum_; ++i)
	{
		delete threadlist_[i];		//删除线程
	}
	threadlist_.clear();			//清空线程列表
}

//启动线程
void ThreadPool::Start()
{
	if(threadnum_ > 0)
	{
		started_ = true;
		for(int i = 0; i < threadnum_; ++i)
		{
			std::thread* pthread = new std::thread(&ThreadPool::ThreadFunc, this);
			threadlist_.push_back(pthread);		//插入到线程列表
		}
	}
	else
	{
		;
	}
}

//暂停线程
void ThreadPool::Stop()
{
	started_ = false;
	condition_.notify_all();	//通知所有线程
}

//signal/broadcast(通知)端
//条件变量的使用有以下四点：
//1. 不一定要在mutex上锁的情况下调用signal，即可把notify()移出临界区外
//2. 在signal之前一定要修改bool表达式
//3. 修改bool表达式通常要用mutex保护
//4. 注意区分signal与broadcast：broadcast通常用于表示状态变化，signal通常用于表示资源可用
void ThreadPool::AddTask(Task task)
{
	{
		std::lock_guard<std::mutex> lock(mutex_);	//上锁
		taskqueue_.push(task);				//加入工作队列
	}
	condition_.notify_one();	//依次唤醒等待队列的线程
}

//wait端
//条件变量的使用有以下三点：
//1. 必须与mutex一起使用
//2. 在mutex已上锁的时候才能调用wait()
//3. 把判断bool条件和wait()放到while循环中
void ThreadPool::ThreadFunc()
{
	std::thread::id tid = std::this_thread::get_id();	//获取线程id
	std::stringstream sin;
	sin << tid;
	std::cout << "worker thread is running :" << tid << std::endl;
	Task task;			
	while(started_)			//started_为true
	{
		task = NULL;
		{
			std::unique_lock<std::mutex> lock(mutex_);	//unique_lock支持解锁又上锁情况
			while(taskqueue_.empty() && started_)		//必须用while循环来等待条件变量，避免虚假唤醒，必须在判断之后再wait()
			{	
				condition_.wait(lock);			//原子地unlock mutex并将线程置于等待(阻塞)状态，不会与工作队列发生死锁
			}						//wait()执行完毕时会自动重新加锁
			if(!started_)
			{
				break;					//started_为false就终止循环
			}
			task = taskqueue_.front();			//移出工作队列
			taskqueue_.pop();
		}
		if(task)
		{
			try
			{
				task();
			}
			catch(std::bad_alloc & ba)	//std::bad_alloc是分配函数作为异常抛出以报告分配存储失败的对象的类型
			{
				std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task:" << ba.what() << '\n';
				while(1);
			}
		}
	}
}
