// Server.cpp : Defines the entry point for the console application.  
//  
#include <iostream> 
#include <thread> //thread 头文件,实现了有关线程的类
#include "winsock2.h"  
#include "MySQLInterface.h"
#include "assign.h"
#include "assignment_data_upload.h"
#include "assignment_data_download.h"
#include "AllocationMessage.h"
#include "tele.h"
#include "ack.h"
using namespace std;

vector<message_buf> MESSAGES;//全局报文池
CRITICAL_SECTION g_CS;//全局任务关键代码段对象
vector<AllocationMessage> ALLOCATION_MESSAGE;//全局任务分配报文池

string MYSQL_SERVER = "";//连接的数据库ip
string MYSQL_USERNAME = "";
string MYSQL_PASSWORD = "";

int main(int argc, char* argv[])
{
	ifstream is("config.txt", ios::in);
	if (!is.is_open()) {
		cout << "| 数据库连接       | ";
		cout << " 无法打开配置文件，请检查config.txt是否配置，并重启系统" << endl;
		//创建不成功释放资源
		system("pause");
		return 0;
	}
	getline(is, MYSQL_SERVER);
	getline(is, MYSQL_USERNAME);
	getline(is, MYSQL_PASSWORD);
	is.close();

	InitializeCriticalSection(&g_CS);//初始化关键代码段对象
    //创建线程
	HANDLE hThread1;//接收任务分配的线程
	hThread1 = CreateThread(NULL, 0, assign, NULL, 0, NULL);
	CloseHandle(hThread1);
	HANDLE hThread2;//接收数据上行的线程
	hThread2 = CreateThread(NULL, 0, assignment_data_upload_rec, NULL, 0, NULL);
	CloseHandle(hThread2);
	HANDLE hThread3;//发送数据ack的线程
	hThread3 = CreateThread(NULL, 0, ack, NULL, 0, NULL);
	CloseHandle(hThread3);
	HANDLE hThread4;//发送数据下行的线程
	hThread4 = CreateThread(NULL, 0, downdata, NULL, 0, NULL);
	CloseHandle(hThread4);
	HANDLE hThread5;//遥测的线程
	hThread5 = CreateThread(NULL, 0, tele, NULL, 0, NULL);
	CloseHandle(hThread5);
	while (1) {
	}

	Sleep(4000);//主线程函数静默4秒
	DeleteCriticalSection(&g_CS);//删除关键代码段对象
	system("pause");
	return 0;
}