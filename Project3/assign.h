/*
任务线程，获取收到的任务
*/
#pragma once
#include <iostream>  
#include <sstream>
#include <string>
#include <fstream>  
#include <vector>
#include <unordered_map>
#include "winsock2.h" 
#include <time.h>
#include <Windows.h>
#include "MySQLInterface.h"
#include "StringNumUtils.h"
#include "Socket.h"
#include <thread> //thread 头文件,实现了有关线程的类
#include "AllocationMessage.h"
#include "DownMessage.h"
#include "AssignmentSocket.h"
using namespace std;

extern string MYSQL_SERVER;
extern string MYSQL_USERNAME;
extern string MYSQL_PASSWORD;
extern vector<message_buf> MESSAGES;//任务分配报文池
extern CRITICAL_SECTION g_CS;//全局关键代码段对象
DWORD WINAPI assign(LPVOID lpParameter);//任务获取线程