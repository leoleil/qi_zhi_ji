#include "assignment_data_download.h"

DWORD downdata(LPVOID lpParameter)
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
		//cout << "| 数据下行         | 监测数据库分配表..." << endl;
		MySQLInterface mysql;//申请数据库连接对象

		//连接数据库
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//从数据中获取分配任务
			//寻找分发标志为2，数据分发标志为0的任务
			string selectSql = "select 主键,任务编号,无人机编号 from 数据下行更新表";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//无任务静默5秒后继续查询
			}
			//查询到任务
			for (int i = 0, len = dataSet.size(); i < len; i++) {
				char* messageDate;
				int messageDataSize = 0;
				//数据下行任务
				StringNumUtils util;//字符转数字工具

				long long dateTime = Message::getSystemTime();//获取当前时间戳
				bool encrypt = false;//是否加密
				UINT32 taskNum = util.stringToNum<UINT32>(dataSet[i][1]);//任务编号
				string ackSql = "";
				char* uavId = new char[20];//无人机编号
				strcpy_s(uavId, dataSet[i][2].size() + 1, dataSet[i][2].c_str());

				//查找地面站ip地址发送报文
				string groundStationSql = "select IP地址 from 服务器信息表";
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					delete uavId;
					continue;//没有找到ip地址
				}

				//创建发送者
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//获取到地址
													 //建立TCP连接
				if (!socketer.createSendServer(ip, 4997, 0)) {
					//创建不成功释放资源
					delete uavId;
					continue;
				}
				MySQLInterface diskMysql;
				if (!diskMysql.connectMySQL(SERVER, USERNAME, PASSWORD, "disk", PORT)) {
					
					cout << "| 数据下行         | 连接数据库失败" << endl;
					cout << "| 数据下行错误信息 | " << diskMysql.errorNum << endl;
					break;
				}
				vector<vector<string>> disk;
				diskMysql.getDatafromDB("SELECT * FROM disk.存盘位置;", disk);
				if (disk.size() == 0) {
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据下行','存盘位置未知',"+ dataSet[i][0] +");");
					cout << "| 数据下行         | 存盘位置未知，请在数据库设置。" << endl;
					mysql.writeDataToDB(ackSql);
					//创建不成功释放资源
					continue;
				}
				string path = disk[0][1];
				path = path + "\\下行传输数据\\" + dataSet[i][2];
				vector<string> files;//要上传的文件
				// 文件句柄
				//long hFile = 0;  //win7
				intptr_t hFile = 0;   //win10
				// 文件信息
				struct _finddata_t fileinfo;
				string p;
				if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
					do {
						if ((strcmp(fileinfo.name, ".") != 0) && (strcmp(fileinfo.name, "..") != 0)) {
							// 保存文件的全路径
							string s = "";
							files.push_back(s.append(fileinfo.name));
						}
						
 					} while (_findnext(hFile, &fileinfo) == 0); //寻找下一个，成功返回0，否则-1

					_findclose(hFile);
				}
				if (files.size() == 0) {
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据下行','无下行文件'," + dataSet[i][0] + ");");
					cout << "| 数据下行         | ";
					cout << path << " 无下行文件" << endl;
					//创建不成功释放资源
					delete uavId;
					continue;
				}
				int pos = files[0].find_last_of('.');
				string fileName(files[0].substr(0,pos));//文件名
				string expandName(files[0].substr(pos));//扩展名
				string file = path.append("\\").append(files[0]);
				ifstream fileIs(file, ios::binary | ios::in);
				if (!fileIs.is_open()) {
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据下行','下行文件无法打开'," + dataSet[i][0] + ");");
					cout << "| 数据下行         | ";
					cout << file << " 无法打开" << endl;
					//创建不成功释放资源
					delete uavId;
					continue;
				}
				cout << "| 数据下行         | ";
				//读取文件
				while (!fileIs.eof()) {
					int bufLen = 1024 * 64;//数据最大64K
					char* fileDataBuf = new char[bufLen];//64K
					fileIs.read(fileDataBuf, bufLen);
					bufLen = fileIs.gcount();//获取实际读取数据大小

					char* up_file_name = new char[32];//文件名
					strcpy_s(up_file_name, fileName.size() + 1, fileName.c_str());
					char* up_expand_name = new char[8];//拓展名
					strcpy_s(up_expand_name, expandName.size() + 1, expandName.c_str());
					bool endFlag = fileIs.eof();//文件尾判断
												//创建数据上行包
					DownMessage downMessage(3020, dateTime, encrypt, taskNum, up_file_name, up_expand_name, endFlag);
					downMessage.setterData(fileDataBuf, bufLen);//传入数据

					const int bufSize = 66560;//发送包固定65k
					int returnSize = 0;
					char* sendBuf = new char[bufSize];//申请发送buf
					ZeroMemory(sendBuf, bufSize);//清空发送空间
					downMessage.createMessage(sendBuf, returnSize, bufSize);//创建传输字节包
					Sleep(10);
					if (socketer.sendMessage(sendBuf, bufSize) == -1) {//发送包固定65k
						//发送失败释放资源跳出文件读写
						mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据下行','发送失败，断开连接'," + dataSet[i][0] + ");");
						cout << "| 数据下行         | 发送失败，断开连接" << endl;
						delete sendBuf;
						delete up_expand_name;
						delete up_file_name;
						delete fileDataBuf;
						break;
					}
					//flieOs.write(fileDataBuf, bufLen);
					cout << ">";
					if (fileIs.eof() == true) {
						cout << endl;
						cout << "| 数据下行         | " << dataSet[i][0] << "号任务下行成功" << endl;
						mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据下行','发送成功断开连接'," + dataSet[i][0] + ");");
						//修改数据库分发标志
						ackSql = "delete from 数据下行更新表 where 主键 = " + dataSet[i][0];
						mysql.writeDataToDB(ackSql);
						
					}

					delete sendBuf;
					delete up_expand_name;
					delete up_file_name;
					delete fileDataBuf;

				}
				//断开TCP
				socketer.offSendServer();
				fileIs.close();
				delete uavId;
			}
			mysql.closeMySQL();

		}
		else {
			cout << "| 数据下行         | 连接数据库失败" << endl;
			cout << "| 数据下行错误信息 | " << mysql.errorNum << endl;
		}
		cout << "| 数据下行         | 任务结束" << endl;

	}
	return 0;
}
