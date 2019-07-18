#include "ack.h"

DWORD ack(LPVOID lpParameter)
{
	//数据库连接关键字
	const char * SERVER = MYSQL_SERVER.data();
	const char * USERNAME = MYSQL_USERNAME.data();
	const char * PASSWORD = MYSQL_PASSWORD.data();
	const char DATABASE[20] = "di_mian_zhan";
	const int PORT = 3306;
	while (1) {
		//5秒监测数据库的任务分配表
		Sleep(5000);
		//cout << "| ACK 发送         | 监测数据库分配表..." << endl;
		MySQLInterface mysql;//申请数据库连接对象

							 //连接数据库
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//从数据中获取分配任务
			string selectSql = "select 任务编号,ACK,unix_timestamp(任务开始时间),unix_timestamp(任务结束时间),卫星编号,服务器编号,任务状态 from 任务分配表 where 任务状态 = 3 or 任务状态 = 5 or 任务状态 = 1;";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//无任务静默5秒后继续查询
			}
			//查询到任务
			for (int i = 0, len = dataSet.size(); i < len; i++) {
				char* messageDate;
				int messageDataSize = 0;
				//if (!dataSet[i][1]._Equal("111")) {

				//	continue;//继续等待
				//}
				StringNumUtils util;//字符转数字工具

				long long dateTime = Message::getSystemTime();//获取当前时间戳
				bool encrypt = false;//是否加密
				UINT32 taskNum = util.stringToNum<UINT32>(dataSet[i][0]);//任务编号
				UINT ACK = util.stringToNum<UINT>(dataSet[i][1]);//ACK
				long long taskStartTime = util.stringToNum<long long>(dataSet[i][2]);//任务开始时间
				long long taskEndTime = util.stringToNum<long long>(dataSet[i][3]);//任务结束时间
				char* satelliteId = new char[20];//卫星编号
				strcpy_s(satelliteId, dataSet[i][4].size() + 1, dataSet[i][4].c_str());
				char* groundStationId = new char[20];//服务器编号
				strcpy_s(groundStationId, dataSet[i][5].size() + 1, dataSet[i][5].c_str());

				//查找地面站ip地址发送报文
				string groundStationSql = "select IP地址 from 服务器信息表 where 服务器编号 =" + dataSet[i][5];
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					delete groundStationId;
					delete satelliteId;
					continue;//没有找到ip地址
				}

				//创建发送者
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//获取到地址
				//建立TCP连接
				if (!socketer.createSendServer(ip, 5000, 0)) {
					//创建不成功释放资源
					delete groundStationId;
					delete satelliteId;
					continue;
				}
				const int bufSize = 66560;//发送包固定65k
				int returnSize = 0;
				char* sendBuf = new char[bufSize];//申请发送buf
				ZeroMemory(sendBuf, bufSize);//清空发送空间
				ACKMessage message(9000, Message::getSystemTime(), false, taskNum, taskStartTime, taskEndTime, ACK);
				message.createMessage(sendBuf, returnSize, bufSize);
				if (socketer.sendMessage(sendBuf, bufSize) == -1) {
					//发送失败
					cout << "| ACK 发送         | 发送失败" << endl;
				}
				string ackSql = "";
				if (dataSet[i][6]._Equal("3")) {
					ackSql = "update 任务分配表 set 任务状态 = 4 where 任务编号 = " + dataSet[i][0];
				}
				else if (dataSet[i][6]._Equal("5")) {
					ackSql = "update 任务分配表 set 任务状态 = 6 where 任务编号 = " + dataSet[i][0];
				}
				else {
					ackSql = "update 任务分配表 set 任务状态 = 1 where 任务编号 = " + dataSet[i][0];
				}
				mysql.writeDataToDB(ackSql);
				delete sendBuf;
				//断开TCP
				socketer.offSendServer();
				delete groundStationId;
				delete satelliteId;
			}
			mysql.closeMySQL();

		}
		else {
			cout << "| ACK 发送         | 连接数据库失败" << endl;
			cout << "| ACK 发送错误信息 | " << mysql.errorNum << endl;
		}
		cout << "| ACK 发送         | 任务结束" << endl;

	}
	return 0;
}
