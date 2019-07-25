#include "assignment_data_upload.h"
vector<message_buf> DATA_MESSAGES;//全局上传数据报文池
CRITICAL_SECTION data_CS;//数据线程池关键代码段对象
DWORD assignment_data_upload_rec(LPVOID lpParameter)
{
	InitializeCriticalSection(&data_CS);//初始化关键代码段对象
	HANDLE hThread1;//创建数据解析线程，读取数据池中数据
	hThread1 = CreateThread(NULL, 0, assignment_data_upload, NULL, 0, NULL);
	CloseHandle(hThread1);
	while (1) {
		UpdataSocket service;//创建接收任务服务
		service.createReceiveServer(4999, DATA_MESSAGES);
	}
	DeleteCriticalSection(&data_CS);//删除关键代码段对象
	return 0;
}

DWORD assignment_data_upload(LPVOID lpParameter)
{
	
	//数据库连接关键字
	const char * SERVER = MYSQL_SERVER.data();
	const char * USERNAME = MYSQL_USERNAME.data();
	const char * PASSWORD = MYSQL_PASSWORD.data();
	const char DATABASE[20] = "di_mian_zhan";
	const int PORT = 3306;
	MySQLInterface mysql;//申请数据库连接对象
	if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
		while (1) {
			Sleep(100);
			EnterCriticalSection(&data_CS);//进入关键代码段
			if (DATA_MESSAGES.empty()) {
				LeaveCriticalSection(&data_CS);
				continue;
			}
			//报文集合大小
			int setLen = DATA_MESSAGES.size();
			LeaveCriticalSection(&data_CS);//离开关键代码段
			while (1) {
				EnterCriticalSection(&data_CS);//进入关键代码段
				char byte_data[70 * 1024];//每个报文空间最大70K
				memcpy(byte_data, DATA_MESSAGES[0].val, 70 * 1024);//将报文池中数据取出
				LeaveCriticalSection(&data_CS);//离开关键代码段
				UpMessage upMessage;
				upMessage.messageParse(byte_data);//解析数据
				char fileName[32];
				upMessage.getterFileName(fileName);
				char expandName[8];
				upMessage.getterExpandName(expandName);
				UINT32 taskNum = upMessage.getterTaskNum();//获取任务编号
				int size = 64 * 1024;
				char* data = new char[size];
				upMessage.getterData(data, size);//获取数据
				StringNumUtils util;
				string taskNumFile = util.numToString<UINT32>(taskNum);
				string ackSql = "";
				ackSql = "update 任务分配表 set 任务状态 = 2 ,任务开始时间 = now() where 任务编号 = " + taskNumFile;
				mysql.writeDataToDB(ackSql);
				MySQLInterface diskMysql;
				if (!diskMysql.connectMySQL(SERVER, USERNAME, PASSWORD, "disk", PORT)) {
					cout << "| 数据上行         | 连接数据库失败" << endl;
					cout << "| 数据上行错误信息 | " << diskMysql.errorNum << endl;
					break;
				}
				vector<vector<string>> disk;
				diskMysql.getDatafromDB("SELECT * FROM disk.存盘位置;", disk);
				if (disk.size() == 0) {
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据上行','存盘位置未知'," + taskNumFile + ");");
					cout << "| 数据上行         | 存盘位置未知，请在数据设置。" << endl;
					break;
				}
				string path = disk[0][1];
				path = path+ "\\上行传输数据\\" + taskNumFile;
				string file_path = path + "\\" + fileName + expandName;
				//string command;
				//command = "mkdir - p " + path;
				//system(command.c_str());//创建文件夹
				if (_access(path.c_str(), 0) == -1) {	//如果文件夹不存在
					_mkdir(path.c_str());				//则创建
					cout << "| 数据上行保存路径 | " + path << endl;

				}
				//打开文件
				ofstream ofs(file_path, ios::binary | ios::out | ios::app);
				ofs.write(data, size);
				ofs.close();
				delete data;
				EnterCriticalSection(&data_CS);//进入关键代码段
				DATA_MESSAGES.erase(DATA_MESSAGES.begin(), DATA_MESSAGES.begin() + 1);//删除元素
				LeaveCriticalSection(&data_CS);

				//判断是否是文件尾
				if (upMessage.getterEndFlag()) {
					//是文件尾要删除缓存数据
					//DATA_MESSAGES.erase(DATA_MESSAGES.begin(), DATA_MESSAGES.begin() + i + 1);
					long long now = Message::getSystemTime();//获取当前时间
					ackSql = "update 任务分配表 set 任务状态 = 3 , ACK = 1000,任务结束时间 = now() where 任务编号 = " + taskNumFile;
					mysql.writeDataToDB(ackSql);
					cout << "| 数据上行         | 已缓存文件下载完毕" << endl;
					mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据上行','已缓存文件下载完毕'," + taskNumFile + ");");
					break;//跳出循环
				}
				else {
					int count = 0;
					//等待新的数据,最多一分钟
					while (count<60) {
						EnterCriticalSection(&data_CS);//进入关键代码段
						if (DATA_MESSAGES.size() > 0) {
							LeaveCriticalSection(&data_CS);
							break;//新数据到达跳出循环
						}
						LeaveCriticalSection(&data_CS);
						count++;
						Sleep(1000);
					}
					if (count == 60) {
						cout << "| 数据上行         | 文件上传数据等待超时" << endl;
						mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据上行','文件上传数据等待超时'," + taskNumFile + ");");
						//删除已经下载数据
						remove(file_path.c_str());
						cout << "| 数据上行         | 清空文件已缓存文件" << endl;
						mysql.writeDataToDB("INSERT INTO 系统日志表(时间,模块,事件,任务编号) VALUES (now(),'数据上行','清空文件已缓存文件'," + taskNumFile + ");");
						long long now = Message::getSystemTime();//获取当前时间
						ackSql = "update 任务分配表 set 任务状态 = 5 , ACK = 1100,任务结束时间 = now()  where 任务编号 = " + taskNumFile;
						mysql.writeDataToDB(ackSql);
						break;//跳出循环
					}

				}
			}

		}
		mysql.closeMySQL();
		
	}
	else {
		cout << "| 数据上行         | 连接数据库失败" << endl;
		cout << "| 数据上行错误信息 | " << mysql.errorNum << endl;
	}
	
	return 0;
}
