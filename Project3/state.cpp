#include "state.h"

DWORD state(LPVOID lpParameter)
{
	//数据库连接关键字
	const char * SERVER = MYSQL_SERVER.data();
	const char * USERNAME = MYSQL_USERNAME.data();
	const char * PASSWORD = MYSQL_PASSWORD.data();
	const char DATABASE[20] = "qian_zhi_ji";
	const int PORT = 3306;
	while (1) {
		//5秒监测数据库的任务分配表
		Sleep(5000);
		MySQLInterface mysql;//申请数据库连接对象

							 //连接数据库
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//从数据中获取分配任务
			string selectSql = "select 主键,无人机编号,无人机状态,unix_timestamp(状态生效时间) from 状态记录表;";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//无任务静默5秒后继续查询
			}
			//查询到任务
			for (int i = 0, len = dataSet.size(); i < len; i++) {
			
				StringNumUtils util;//字符转数字工具

				long long dateTime = Message::getSystemTime();//获取当前时间戳
				bool encrypt = false;//是否加密
				UINT state = util.stringToNum<UINT>(dataSet[i][2]);//状态
				long long stateStartTime = util.stringToNum<long long>(dataSet[i][3]);//状态生效时间
				char* uavNo = new char[20];//无人机编号
				strcpy_s(uavNo, dataSet[i][1].size() + 1, dataSet[i][1].c_str());

				//查找地面站ip地址发送报文
				string groundStationSql = "select IP地址 from 服务器信息表";
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					continue;//没有找到ip地址
				}

				//创建发送者
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//获取到地址
													 //建立TCP连接
				if (!socketer.createSendServer(ip, 5002, 0)) {
					//创建不成功释放资源
					continue;
				}
				const int bufSize = 66560;//发送包固定65k
				int returnSize = 0;
				char* sendBuf = new char[bufSize];//申请发送buf
				ZeroMemory(sendBuf, bufSize);//清空发送空间
				StateMessage message(3010, Message::getSystemTime(), false, uavNo, stateStartTime, state);
				delete uavNo;//释放资源
				message.createMessage(sendBuf, returnSize, bufSize);
				if (socketer.sendMessage(sendBuf, bufSize) == -1) {
					//发送失败
					cout << "| 状态发送         | 发送失败" << endl;
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'状态发送','发送失败'," + dataSet[i][0] + ");");
				}
				string ackSql = "delete from 状态记录表 where 主键 = " + dataSet[i][0];
				
				cout << "| 状态发送         | 发送成功" << endl;
				mysql.writeDataToDB(ackSql);
				delete sendBuf;
				//断开TCP
				socketer.offSendServer();
			}
			mysql.closeMySQL();

		}
		else {
			cout << "| 状态发送         | 连接数据库失败" << endl;
			cout << "| 状态发送错误信息 | " << mysql.errorNum << endl;
		}
		cout << "| 状态发送         | 任务结束" << endl;

	}
	return 0;
}
