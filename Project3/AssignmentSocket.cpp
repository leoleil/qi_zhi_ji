#include "AssignmentSocket.h"
#include "AllocationMessage.h"
#include "MySQLInterface.h"
#include <iostream> 
#include <sstream>
#include <string>
using namespace std;
extern string MYSQL_SERVER;
extern string MYSQL_USERNAME;
extern string MYSQL_PASSWORD;
AssignmentSocket::AssignmentSocket()
{
}


AssignmentSocket::~AssignmentSocket()
{
}

int AssignmentSocket::createReceiveServer(const int port, std::vector<message_buf>& message)
{
	cout << "| 任务分配         | 服务启动" << endl;
	//初始化套结字动态库  
	if (WSAStartup(MAKEWORD(2, 2), &S_wsd) != 0)
	{
		cout << "WSAStartup failed!" << endl;
		return 1;
	}

	//创建套接字  
	sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sServer)
	{
		cout << "socket failed!" << endl;
		WSACleanup();//释放套接字资源;  
		return  -1;
	}

	//服务器套接字地址   
	addrServ.sin_family = AF_INET;
	addrServ.sin_port = htons(port);
	addrServ.sin_addr.s_addr = INADDR_ANY;
	//绑定套接字  
	retVal = bind(sServer, (LPSOCKADDR)&addrServ, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR == retVal)
	{
		cout << "bind failed!" << endl;
		closesocket(sServer);   //关闭套接字  
		WSACleanup();           //释放套接字资源;  
		return -1;
	}

	//开始监听   
	cout << "| 任务分配         | listening" << endl;
	retVal = listen(sServer, 1);

	if (SOCKET_ERROR == retVal)
	{
		cout << "listen failed!" << endl;
		closesocket(sServer);   //关闭套接字  
		WSACleanup();           //释放套接字资源;  
		return -1;
	}

	//接受客户端请求  
	sockaddr_in addrClient;
	int addrClientlen = sizeof(addrClient);
	sClient = accept(sServer, (sockaddr FAR*)&addrClient, &addrClientlen);
	if (INVALID_SOCKET == sClient)
	{
		cout << "accept failed!" << endl;
		closesocket(sServer);   //关闭套接字  
		WSACleanup();           //释放套接字资源;  
		return -1;
	}

	cout << "| 任务分配         | TCP连接创建" << endl;
	while (true) {
		//数据窗口
		const int data_len = 66560;//每次接收65K数据包
		char data[66560]; //数据包
		ZeroMemory(data, data_len);//将数据包空间置0
		char* data_ptr = data;//数据指针
		int r_len = 0;
		while (1) {
			//接收客户端数据
			//清空buffer
			ZeroMemory(buf, BUF_SIZE);

			//获取数据
			retVal = recv(sClient, buf, BUF_SIZE, 0);

			if (SOCKET_ERROR == retVal)
			{
				cout << "| 任务分配         | 接收程序出错" << endl;
				closesocket(sServer);   //关闭套接字    
				closesocket(sClient);   //关闭套接字
				return -1;
			}
			if (retVal == 0) {
				cout << "| 任务分配         | 接收完毕断开本次连接" << endl;
				closesocket(sServer);   //关闭套接字    
				closesocket(sClient);   //关闭套接字
				return -1;
			}
			memcpy(data_ptr, buf, retVal);
			r_len = r_len + retVal;

			data_ptr = data_ptr + retVal;
			if ((data_ptr - data) >= data_len) {
				break;//如果接收到的数据大于最大窗口跳出循环（数据包64K）
			}

		}


		//将获取到的数据放入数据库中
		const char * SERVER = MYSQL_SERVER.data();//连接的数据库ip
		const char * USERNAME = MYSQL_USERNAME.data();
		const char * PASSWORD = MYSQL_PASSWORD.data();
		const char DATABASE[20] = "di_mian_zhan";
		const int PORT = 3306;
		MySQLInterface mysql;
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			char* ptr = data;
			UINT32 length = 0;
			for (int i = 0; i < data_len; i = i + length) {

				if (data[i] == NULL && data[i + 1] == NULL)break;
				//获取报文长度
				memcpy(&length, ptr + i + 2, 4);

				//获取一个buffer
				char val[65 * 1024];
				//内存复制
				memcpy(val, ptr + i, length);
				//解析报文写入数据库
				AllocationMessage message;
				message.messageParse(val);
				//如果是撤销报文
				if (message.getterMessageId() == 4020) {
					string sql = "SELECT * FROM di_mian_zhan.任务分配表 where 任务状态 = 0 and 任务编号 = ";
					sql = sql + to_string(message.getterTaskNum()) + ";";
					vector<vector<string>> s;
					mysql.getDatafromDB(sql, s);
					if (s.size() == 0) {
						cout << "| 任务分配         | 撤销失败，任务号为" << to_string(message.getterTaskNum()) << endl;
						continue;
					}
					sql = "UPDATE `di_mian_zhan`.`任务分配表`SET`任务状态` = 1,`ACK` = 1200 WHERE `任务编号` = ";
					sql = sql + to_string(message.getterTaskNum()) + ";";
					mysql.writeDataToDB(sql);
					cout << "| 任务分配         | 撤销成功，任务号为" << to_string(message.getterTaskNum()) << endl;
				}
				else
				{
					int size;
					char satrlliteId[20];
					message.getterSatelliteId(satrlliteId, size);
					string sql = "INSERT INTO `di_mian_zhan`.`任务分配表`(`任务获取时间`,`任务编号`,`卫星编号`,`任务类型`,`计划开始时间`,`计划截止时间`,`任务状态`)VALUES(now(),";
					sql = sql + to_string(message.getterTaskNum()) + ",'" + satrlliteId + "'," + to_string(message.getterTaskType()) + ",from_unixtime(" + to_string(message.getterTaskStartTime()) + "),from_unixtime(" + to_string(message.getterTaskEndTime()) + "),0);";
					mysql.writeDataToDB(sql);
					if (message.getterTaskType() == 101) {
						//如果是遥控报文，添加遥控信息
						int message_date_len = 64 * 1024;
						char message_date[64 * 1024];
						message.getterMessage(message_date, message_date_len);
						sql = "update `di_mian_zhan`.`任务分配表` set `遥控信息` = '";
						sql = sql + message_date + "' where `任务编号` = " + to_string(message.getterTaskNum()) + ";";
						mysql.writeDataToDB(sql);
					}

					cout << "| 任务分配         | 成功" << endl;
				}
				mysql.closeMySQL();
			}
		}
		else {
			cout << "| 任务分配         | 连接数据库失败" << endl;
			cout << "| 任务分配错误信息 | " << mysql.errorNum << endl;
		}

	}
	//退出  
	closesocket(sServer);   //关闭套接字  
	closesocket(sClient);   //关闭套接字  

	return 0;
}
