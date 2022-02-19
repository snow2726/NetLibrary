//
//HttpSession类：用于解析Http消息报文

#ifndef _HTTP_SESSION_H_
#define _HTTP_SESSION_H_

#include <sstream>
#include <string>
#include <map>

//请求报文
typedef struct _HttpRequestContext {
	std::string method;				//方法字段：GET、POST等
	std::string url;				//URL字段
	std::string version;				//版本字段
	std::map<std::string, std::string> header;	//首部行
	std::string body;				//实体体
	
}HttpRequestContext;

//响应报文
typedef struct _HttpResponseContext {
	std::string version;				//版本字段
	std::string statecode;				//状态码
	std::string statemsg;				//相应状态信息
	std::map<std::string, std::string> header;	//首部行
	std::string body;				//实体体
}HttpResponseContext;

class HttpSession
{
public:
	//构造函数与析构函数
	HttpSession();
	~HttpSession();
	
	//解析HTTP报文
	bool PraseHttpRequest(std::string& s, HttpRequestContext& httprequestcontext);
	
	//处理报文
	void HttpProcess(const HttpRequestContext& httprequestcontext, std::string& responsecontext);
	
	//错误消息报文组装，404等
	void HttpError(const int err_num, const std::string short_msg, const HttpRequestContext& httprequestcontext, std::string& responsecontext);
	
	//判断长连接
	bool KeepAlive()
	{
		return keepalive_;
	}	

private:
	//请求报文相关成员
	HttpRequestContext httprequestcontext_;
	bool praseresult_;

	//响应报文相关成员
	std::string responsecontext_;
	std::string responsebody_;
	std::string errormsg_;
	std::string path_;
	std::string querystring_;
	
	//长连接标志
	bool keepalive_;
	//报文实体缓冲区
	std::string body_buff; 

};

#endif
