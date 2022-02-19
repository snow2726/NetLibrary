//
//EventLoopThreadPool类
//采用Round-Robin策略来选取线程池中的EventLoop，不允许TcpConnection在运行中更换EvnetLoop
//每个TcpServer有自己的EventLoopThreadPool，多个TcpServer之间不共享EventLoopThreadPool
//


#include "EventLoopThreadPool.h"

//构造函数
EventLoopThreadPool::EventLoopThreadPool(EventLoop *mainloop, int threadnum)
		:mainloop_(mainloop),
		threadnum_(threadnum),
		threadlist_(),
		index_(0)
{
	for(int i=0; i<threadnum_; ++i)		//轮询
	{	
		//创建新的线程，并加入到线程列表
		EventLoopThread *peventloopthread = new EventLoopThread;
		threadlist_.push_back(peventloopthread);
	}
}

//析构函数
EventLoopThreadPool::~EventLoopThreadPool()
{
	std::cout << "Clean up the EventLoopThreadPool" << std::endl;
	for(int i=0; i<threadnum_; ++i)
	{
		delete threadlist_[i];
	}
	threadlist_.clear();
}

//启动线程池
void EventLoopThreadPool::Start()
{
	if(threadnum_ > 0)
	{
		for(int i=0; i<threadnum_; ++i)
		{
			threadlist_[i]->Start();	//启动每个线程
		}
	}
	else
	{
		;
	}
}

//获取下一个被分发的loop，依据RR轮询策略
EventLoop* EventLoopThreadPool::GetNextLoop()
{
	if(threadnum_ > 0)	//多线程
	{
		EventLoop *loop = threadlist_[index_]->GetLoop();	//通过索引去对线程池里的线程进行事件分发
		index_ = (index_ + 1) % threadnum_;	//取模
		return loop;
	}
	else			//单线程
	{
		return mainloop_;	//就返回mainloop_，即TcpServer自己用的那个loop
	}
}
