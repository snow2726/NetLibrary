//
//服务器socket类，封装socket描述符及相关的初始化操作
//

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>	//Internet 地址族
#include <arpa/inet.h>	//Internet操作的定义

class Socket{
public:
	//构造函数与析构函数
	Socket();
	~Socket();
	
	//获取fd
	int fd() const { return serverfd_; }
	
	//Socket设置
	void SetSocketOption();
	
	//设置地址重用
	void SetReuseAddr();
	
	//设置非阻塞
	void Setnonblocking();
	
	//绑定地址
	bool BindAddress( int serverport );
	
	//开启监听
	bool Listen();
	
	//accept获取连接
	int Accept( struct sockaddr_in &clientaddr );
	
	//关闭服务器fd
	bool Close();
private:
	//服务器Socket文件描述符
	int serverfd_;
	
};

#endif
