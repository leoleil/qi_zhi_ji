#include "ack.h"

DWORD ack(LPVOID lpParameter)
{
	//数据库连接关键字
	const char * SERVER = MYSQL_SERVER.data();
	const char * USERNAME = MYSQL_USERNAME.data();
	const char * PASSWORD = MYSQL_PASSWORD.data();
	const char DATABASE[20] = "qian_zhi_ji";
	const int PORT = 3306;
	while (1) {
		//5秒监测数据库的任务信息表
		Sleep(5000);
		//cout << "| ACK 发送         | 监测数据库分配表..." << endl;
		MySQLInterface mysql;//申请数据库连接对象

							 //连接数据库
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//从数据中获取分配任务
			string selectSql = "select 任务编号,unix_timestamp(上注时间),任务状态 from 任务信息表 where 任务状态 = 2 || 任务状态 = 4 || 任务状态 = 6";
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
				UINT ACK = 1300;//ACK
				if (dataSet[i][2]._Equal("2")) {
					ACK = 1300;//ACK
				}
				else if (dataSet[i][2]._Equal("4")) {
					ACK = 1400;//ACK
				}
				else {
					ACK = 1500;//ACK
				}
				long long taskStartTime = util.stringToNum<long long>(dataSet[i][1]);//上注时间

				//查找地面站ip地址发送报文
				string groundStationSql = "select IP地址 from 服务器信息表;";
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					continue;//没有找到ip地址
				}

				//创建发送者
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//获取到地址
				//建立TCP连接
				if (!socketer.createSendServer(ip, 5000, 0)) {
					//创建不成功释放资源
					continue;
				}
				const int bufSize = 66560;//发送包固定65k
				int returnSize = 0;
				char* sendBuf = new char[bufSize];//申请发送buf
				ZeroMemory(sendBuf, bufSize);//清空发送空间
				ACKMessage message(9000, Message::getSystemTime(), false, taskNum, taskStartTime, ACK);
				message.createMessage(sendBuf, returnSize, bufSize);
				if (socketer.sendMessage(sendBuf, bufSize) == -1) {
					//发送失败
					cout << "| ACK 发送         | 发送失败" << endl;
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'ACK 发送','发送失败',"+ dataSet[i][0] +");");
				}
				string ackSql = "";
				if (dataSet[i][2]._Equal("2")) {
					ackSql = "update 任务信息表 set 任务状态 = 3 where 任务编号 = " + dataSet[i][0];
				}
				else if (dataSet[i][2]._Equal("4")) {
					ackSql = "update 任务信息表 set 任务状态 = 5 where 任务编号 = " + dataSet[i][0];
				}
				else {
					ackSql = "update 任务信息表 set 任务状态 = 7 where 任务编号 = " + dataSet[i][0];
				}
				cout << "| ACK 发送         | 发送成功" << endl;
				mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'ACK 发送','发送成功'," + dataSet[i][0] + ");");
				mysql.writeDataToDB(ackSql);

				delete sendBuf;
				//断开TCP
				socketer.offSendServer();
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
