
#定义变量
DIR_SRC = ../NetLibrary


#wildcard 函数用来在 Makefile 中，替换 Bash 的通配符
SRC = $(wildcard $(DIR_SRC)/*.cpp)



#定义变量
CXX_FLAG = -g -Wall -std=c++11 -pthread -O3

CC = g++


TARGET = httpserver


#编译文件的规则
$(DIR_SRC)/$(TARGET) : *.cpp

	$(CC) $(CXX_FLAG) -o $@ $^


#编译文件的规则
$(DIR_SRC)/%.o : $(DIR_SRC)/%.cpp

	$(CC) $(CXX_FLAG) -c $< -o $@


#定义伪目标clean，用于清除运行后的环境
.PHONY : clean

clean : 
	-rm -rf httpserver 
