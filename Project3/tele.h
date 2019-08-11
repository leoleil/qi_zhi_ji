#pragma once
#pragma warning(disable:4996)
#define _CRT_SECURE_NO_WARNINGS 1
#include <iostream>  
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "winsock2.h" 
#include <time.h>
#include <Windows.h>
#include "MySQLInterface.h"
#include "Message.h"
#include "DownMessage.h"
#include "StringNumUtils.h"
#include "Socket.h"

using namespace std;

extern string MYSQL_SERVER;
extern string MYSQL_USERNAME;
extern string MYSQL_PASSWORD;
//全局变量
const int M = 1048576;				//1M									
const int K = 1024;					//1K
const double NEW_NULL = 1000000000;//这个数代表NULL
const int BUF_SIZE = M;

//字段定义
typedef struct field {
	string f_name;
	INT16 type;
	double min = NEW_NULL;
	double max = NEW_NULL;
	string unit;
	bool display;
}field;
//定义报文
typedef struct definitionMes {
	string id;//无人机编号
	string name;
	string p_name;
	vector<field> f_list;
}definitionMes;

typedef struct mydata {
	int len = 0;
	char* val = new char[K];
}mydata;

/*遥测模块*/
DWORD WINAPI tele(LPVOID lpParameter);
DWORD WINAPI tele2(LPVOID lpParameter);
