#include "tele.h"

DWORD tele(LPVOID lpParameter)
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
		//cout << "| 卫星遥测         | 监测数据库分配表..." << endl;
		MySQLInterface mysql;//申请数据库连接对象

							 //连接数据库
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//从数据中获取分配任务
			//寻找100任务类型
			string selectSql = "select 任务编号,任务类型,unix_timestamp(计划开始时间),unix_timestamp(计划截止时间),卫星编号,服务器编号 from 任务分配表 where 任务状态 = 0 and 任务类型 = 100";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//无任务静默5秒后继续查询
			}
			//查询到任务
			for (int i = 0, len = dataSet.size(); i < len; i++) {
				char* messageDate;
				int messageDataSize = 0;
				if (!dataSet[i][1]._Equal("100")) {
					continue;//继续等待
				}
				StringNumUtils util;//字符转数字工具
				vector<definitionMes> D_MES_LIST;//每一个设备定义数据报文定义集合

				long long dateTime = Message::getSystemTime();//获取当前时间戳
				bool encrypt = false;//是否加密
				UINT32 taskNum = util.stringToNum<UINT32>(dataSet[i][0]);//任务编号
				UINT16 taskType = util.stringToNum<UINT16>(dataSet[i][1]);//任务类型
				long long taskStartTime = util.stringToNum<long long>(dataSet[i][2]);//计划开始时间
				if (taskStartTime *1000 > dateTime) {
					//如果还没到计划开始时间就跳过
					continue;
				}
				string ackSql = "";
				if (i == 0) {
					//将该任务号的状态改为正在执行，并且记录任务开始时间
					ackSql = "update 任务分配表 set 任务状态 = 2 ,任务开始时间 = now() where 任务编号 = " + dataSet[i][0];
					mysql.writeDataToDB(ackSql);
				}

				long long taskEndTime = util.stringToNum<long long>(dataSet[i][3]);//计划截止时间
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

				//创建发送者，与服务器创建连接
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//获取到地址
				//建立TCP连接
				if (!socketer.createSendServer(ip, 4996, 0)) {
					//创建不成功释放资源
					delete groundStationId;
					delete satelliteId;
					continue;
				}

				//开始读取配置表
				selectSql = "SELECT 设备名, 父设备名 FROM di_mian_zhan.设备关系表 WHERE 卫星编号 = '";
				selectSql = selectSql + satelliteId + "';";
				//从数据库中读取设备关系表和字段定义表
				vector<vector<string>> str_ralation_Array;//输入设备关系集合
				mysql.getDatafromDB(selectSql, str_ralation_Array);
				vector<vector<string>> str_definition_Array;//输入字段定义集合
				selectSql = "SELECT ID,字段名,数据类型,最大值,最小值,单位,显示标志,设备名 FROM di_mian_zhan.字段定义表 where 卫星编号 = '";
				selectSql = selectSql + satelliteId + "';";
				mysql.getDatafromDB(selectSql, str_definition_Array);

				StringNumUtils utiles;
				for (int i = 0; i < str_ralation_Array.size(); i++) {
					definitionMes mes;
					mes.name = str_ralation_Array[i][0];
					mes.p_name = str_ralation_Array[i][1];
					for (int j = 0; j < str_definition_Array.size(); j++) {
						if (str_definition_Array[j][7] == str_ralation_Array[i][0]) {
							field myfield;
							myfield.f_name = str_definition_Array[j][1];
							myfield.type = utiles.stringToNum<INT16>(str_definition_Array[j][2]);
							if (str_definition_Array[j][3] != "")myfield.min = utiles.stringToNum<double>(str_definition_Array[j][3]);
							if (str_definition_Array[j][4] != "")myfield.max = utiles.stringToNum<double>(str_definition_Array[j][4]);
							myfield.unit = str_definition_Array[j][5];
							myfield.display = utiles.stringToNum<bool>(str_definition_Array[j][6]);
							mes.f_list.push_back(myfield);
						}
					}
					D_MES_LIST.push_back(mes);
				}
				//检查卫星更新表如果有新卫星就发送定义报文没有则发送数据报文
				selectSql = "SELECT 卫星编号 FROM di_mian_zhan.卫星更新表;";
				vector<vector<string>> satellite_id;
				mysql.getDatafromDB(selectSql, satellite_id);
				if (satellite_id.size() != 0) {
					if ( satellite_id[0][0] == dataSet[i][4]) {
						//发送定义报文
						cout << "| 卫星遥测         | 发送定义报文>";
						for (int i = 0; i < D_MES_LIST.size(); i++) {
							//报文长度
							UINT32 message_len = 0;
							BYTE message[M];
							BYTE* ptr = message;//message指针
							UINT16 mesage_id = 1000;//报文标识
							long long timestamp = Message::getSystemTime();//时间戳
							bool encryption = false;//加密标识
							char name[40];//设备名称
							strcpy_s(name, 40, D_MES_LIST[i].name.c_str());
							char parent_name[40];//父设备名称
							strcpy_s(parent_name, 40, D_MES_LIST[i].p_name.c_str());
							//包装message
							memcpy(ptr, &mesage_id, sizeof(UINT16));//设备标识
							ptr = ptr + sizeof(UINT16);
							memcpy(ptr, &message_len, sizeof(UINT32));//字段长度
							ptr = ptr + sizeof(UINT32);
							memcpy(ptr, &timestamp, sizeof(long long));//时间
							ptr = ptr + sizeof(long long);
							memcpy(ptr, &encryption, sizeof(bool));//加密
							ptr = ptr + sizeof(bool);
							memcpy(ptr, &taskNum, sizeof(UINT32));//任务号
							ptr = ptr + sizeof(UINT32);
							memcpy(ptr, name, 40);//设备名
							ptr = ptr + 40;
							memcpy(ptr, parent_name, 40);//父设备名
							ptr = ptr + 40;
							memcpy(ptr, satelliteId, 20);//卫星编号
							ptr = ptr + 20;
							for (int j = 0; j < D_MES_LIST[i].f_list.size(); j++) {
								char field_name[40];//字段名称
								UINT16 field_type;//字段类型编号
								double min = NULL;//最小值
								double max = NULL;//最大值
								char unit[8];//单位
								bool display;//显示标志
								strcpy_s(field_name, 40, D_MES_LIST[i].f_list[j].f_name.c_str());
								field_type = D_MES_LIST[i].f_list[j].type;
								min = D_MES_LIST[i].f_list[j].min;
								max = D_MES_LIST[i].f_list[j].max;
								strcpy_s(unit, 8, D_MES_LIST[i].f_list[j].unit.c_str());
								display = D_MES_LIST[i].f_list[j].display;
								memcpy(ptr, field_name, 40);
								ptr = ptr + 40;
								memcpy(ptr, &field_type, sizeof(UINT16));
								ptr = ptr + sizeof(UINT16);
								memcpy(ptr, &min, sizeof(double));
								ptr = ptr + sizeof(double);
								memcpy(ptr, &max, sizeof(double));
								ptr = ptr + sizeof(double);
								memcpy(ptr, unit, 8);
								ptr = ptr + 8;
								memcpy(ptr, &display, sizeof(bool));
								ptr = ptr + sizeof(bool);
							}
							Sleep(10);
							message_len = ptr - message;
							memcpy(message + 2, &message_len, sizeof(UINT32));
							const int bufSize = 66560;//发送包固定65k
							int returnSize = 0;
							char sendBuf[bufSize];//申请发送buf
							ZeroMemory(sendBuf, bufSize);//清空发送空间
							memcpy(sendBuf, message, message_len);
							if (socketer.sendMessage(sendBuf, bufSize) == -1) {//发送包固定65k

								//发送失败释放资源跳出文件读写
								cout << "| 卫星遥测         | 定义报文发送失败" << endl;
								break;
							}
							cout << ">";

						}
						cout << endl;
						//定义报文发送完毕
						//删除卫星更新信息
						selectSql = "DELETE FROM `di_mian_zhan`.`卫星更新表` WHERE (`卫星编号` = '";
						selectSql = selectSql + satellite_id[0][0] + "');";
						mysql.writeDataToDB(selectSql);
						
					}
					else {
						//如果新的数据和任务的不匹配则删除
						selectSql = "DELETE FROM `di_mian_zhan`.`卫星更新表` WHERE (`卫星编号` = '";
						selectSql = selectSql + satellite_id[0][0] + "');";
						mysql.writeDataToDB(selectSql);
					}
				}
				//检查是否有数据下来
				while (true)
				{
					//有数据下来则发送数据报文
					selectSql = "SELECT 卫星编号,设备名 FROM di_mian_zhan.遥测数据更新表;";
					vector<vector<string>> new_data_set;
					mysql.getDatafromDB(selectSql, new_data_set);
					if (new_data_set.size() != 0) {
						for (int i = 0; i < new_data_set.size(); i++) {
							//检查卫星编号
							if (!(new_data_set[i][0] == dataSet[i][4])) continue;//不是该卫星编号的数据跳过
							
							selectSql = "SELECT * FROM ";
							selectSql = selectSql + new_data_set[i][0] + "_" + new_data_set[i][1] + " where 发送标志 = 0;";
							vector<vector<string>> satellite_data;//数据
							mysql.getDatafromDB(selectSql, satellite_data);
							//发送数据报文
							cout << "| 卫星遥测         | 发送数据报文>";
							for (int ii = 0; ii < satellite_data.size(); ii++) {
								vector<string> lineArray = satellite_data[ii];
								BYTE buf[M];//数据buf
								BYTE* bufPtr = buf;//数据报文指针
								BYTE* buf_len_Ptr = buf;
								UINT16 id = 2000;//报文标识
								UINT32 len = 0;//报文长度
								memcpy(bufPtr, &id, sizeof(UINT16));//报文标识
								bufPtr = bufPtr + sizeof(UINT16);
								len = len + sizeof(UINT16);
								memcpy(bufPtr, &len, sizeof(UINT32));//报文长度
								bufPtr = bufPtr + sizeof(UINT32);
								len = len + sizeof(UINT16);
								long long timestamp = utiles.stringToNum<long long>(lineArray[1]);//产生时间
								memcpy(bufPtr, &timestamp, sizeof(long long));
								len = len + sizeof(long long);
								bufPtr = bufPtr + sizeof(long long);
								bool flag = 0;//加密标识
								memcpy(bufPtr, &flag, sizeof(bool));
								bufPtr = bufPtr + sizeof(bool);
								len = len + sizeof(bool);
								memcpy(bufPtr, &taskNum, sizeof(UINT32));//任务编号
								bufPtr = bufPtr + sizeof(UINT32);
								len = len + sizeof(bool);
								char name[40];//设备名称
								strcpy_s(name, 40, new_data_set[i][1].c_str());
								memcpy(bufPtr, name, 40);
								bufPtr = bufPtr + 40;
								len = len + 40;
								memcpy(bufPtr, satelliteId, 20);//卫星编号
								bufPtr = bufPtr + 20;
								long long tm = Message::getSystemTime();//采样时间
								memcpy(bufPtr, &tm, sizeof(long long));
								bufPtr = bufPtr + sizeof(long long);
								//通过设备名去定位该设备的字段信息
								for (int j = 0; j < D_MES_LIST.size(); j++) {
									if (D_MES_LIST[j].name == new_data_set[i][1]) {//如果找到了该数据的定义报文
										for (int k = 0; k < D_MES_LIST[j].f_list.size(); k++) {//在其定义报文字段查找类型并写入Buffer
											if (D_MES_LIST[j].f_list[k].type == 1) {//16位整数
												INT16 data1 = utiles.stringToNum<INT16>(lineArray[k + 2]);
												memcpy(bufPtr, &data1, sizeof(INT16));
												bufPtr = bufPtr + sizeof(INT16);
												len = len + sizeof(INT16);
											}
											else if (D_MES_LIST[j].f_list[k].type == 2) {//16位无符号整数
												UINT16 data2 = utiles.stringToNum<UINT16>(lineArray[k + 2]);
												memcpy(bufPtr, &data2, sizeof(UINT16));
												bufPtr = bufPtr + sizeof(UINT16);
												len = len + sizeof(UINT16);
											}
											else if (D_MES_LIST[j].f_list[k].type == 3) {//32位整数
												INT32 data3 = utiles.stringToNum<INT32>(lineArray[k + 2]);
												memcpy(bufPtr, &data3, sizeof(INT32));
												bufPtr = bufPtr + sizeof(INT32);
												len = len + sizeof(INT32);
											}
											else if (D_MES_LIST[j].f_list[k].type == 4) {//32位无符号整数
												UINT32 data4 = utiles.stringToNum<UINT32>(lineArray[k + 2]);
												memcpy(bufPtr, &data4, sizeof(UINT32));
												bufPtr = bufPtr + sizeof(UINT32);
												len = len + sizeof(UINT32);
											}
											else if (D_MES_LIST[j].f_list[k].type == 5) {//64位整数
												INT64 data5 = utiles.stringToNum<INT64>(lineArray[k + 2]);
												memcpy(bufPtr, &data5, sizeof(INT64));
												bufPtr = bufPtr + sizeof(INT64);
												len = len + sizeof(INT64);
											}
											else if (D_MES_LIST[j].f_list[k].type == 6) {//64位无符号整数
												UINT64 data6 = utiles.stringToNum<UINT64>(lineArray[k + 2]);
												memcpy(bufPtr, &data6, sizeof(UINT64));
												bufPtr = bufPtr + sizeof(UINT64);
												len = len + sizeof(UINT64);
											}
											else if (D_MES_LIST[j].f_list[k].type == 7) {//单精度浮点数值
												float data7 = utiles.stringToNum<float>(lineArray[k + 2]);
												memcpy(bufPtr, &data7, sizeof(float));
												bufPtr = bufPtr + sizeof(float);
												len = len + sizeof(float);
											}
											else if (D_MES_LIST[j].f_list[k].type == 8) {//双精度浮点数值
												double data8 = utiles.stringToNum<double>(lineArray[k + 2]);
												memcpy(bufPtr, &data8, sizeof(double));
												bufPtr = bufPtr + sizeof(double);
												len = len + sizeof(double);
											}
											else if (D_MES_LIST[j].f_list[k].type == 9) {//字符类型
												char data9 = lineArray[k + 2].data()[0];
												memcpy(bufPtr, &data9, sizeof(char));
												bufPtr = bufPtr + sizeof(char);
												len = len + sizeof(char);
											}
											else if (D_MES_LIST[j].f_list[k].type == 10) {//短字符串类型
												char buf_data[15];//数据包
												strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
												int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
												memcpy(bufPtr, lineArray[k + 2].data(), size);
												bufPtr = bufPtr + 15;
												len = len + 15;
											}

											else if (D_MES_LIST[j].f_list[k].type == 11) {//长字符串类型
												char buf_data[255];//数据包
												strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
												int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
												memcpy(bufPtr, buf_data, size);
												bufPtr = bufPtr + 255;
												len = len + 255;
											}
											else if (D_MES_LIST[j].f_list[k].type == 12) {//短字节数组255
												char buf_data[255];//数据包
												strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
												int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
												memcpy(bufPtr, buf_data, size);
												bufPtr = bufPtr + 255;
												len = len + 255;
											}
											else if (D_MES_LIST[j].f_list[k].type == 13) {//中字节数组32K
												char buf_data[32 * K];//数据包
												strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
												int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
												memcpy(bufPtr, buf_data, size);
												bufPtr = bufPtr + 32 * K;
												len = len + 32 * K;
											}
											else if (D_MES_LIST[j].f_list[k].type == 14) {//长字节数组64K
												char buf_data[60 * K];//数据包
												strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
												int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
												memcpy(bufPtr, buf_data, size);
												int add = 60 * K;
												bufPtr = bufPtr + add;
												len = len + 60 * K;

											}
											else if (D_MES_LIST[j].f_list[k].type == 15) {//BOOL
												bool data = utiles.stringToNum<bool>(lineArray[k + 2]);
												memcpy(bufPtr, &data, sizeof(bool));
												bufPtr = bufPtr + sizeof(bool);
												len = len + sizeof(bool);
											}
											else if (D_MES_LIST[j].f_list[k].type == 16) {//时间戳
												long long data16 = utiles.stringToNum<long long>(lineArray[k + 2]);
												memcpy(bufPtr, &data16, sizeof(long long));
												bufPtr = bufPtr + sizeof(long long);
												len = len + sizeof(long long);
											}
										}
									}
								}
								Sleep(10);//按照设置的速率发送
								len = bufPtr - buf_len_Ptr;
								memcpy(buf_len_Ptr + 2, &len, sizeof(UINT32));
								const int bufSize = 66560;//发送包固定65k
								int returnSize = 0;
								char* sendBuf = new char[bufSize];//申请发送buf
								ZeroMemory(sendBuf, bufSize);//清空发送空间
								memcpy(sendBuf, buf, len);
								if (socketer.sendMessage(sendBuf, bufSize) == -1) {//发送包固定65k
									//发送失败释放资源跳出文件读写
									cout << "| 卫星遥测         | 数据报文发送失败" << endl;
									delete sendBuf;

									break;
								}
								cout << ">";
								//删除数据
								ackSql = "DELETE FROM ";
								ackSql = ackSql + new_data_set[i][0] + "_" + new_data_set[i][1] + " where 主键 = " + satellite_data[ii][0] + ";";
								mysql.writeDataToDB(ackSql);
							}
							cout << endl;
							ackSql = "DELETE FROM di_mian_zhan.遥测数据更新表 where 卫星编号 = '" + new_data_set[i][0] + "' and 设备名 = '" + new_data_set[i][1] + "';";
							mysql.writeDataToDB(ackSql);
						}
					}
					Sleep(1000);

					if (Message::getSystemTime() > taskEndTime*1000)break;//如果到了计划结束时间则结束发送
					
					
				}
				//修改数据库分发标志
				ackSql = "update 任务分配表 set 任务状态 = 3 , ACK = 1000,任务结束时间 = now()  where 任务编号 = " + dataSet[i][0];
				mysql.writeDataToDB(ackSql);
				mysql.closeMySQL();
				
				////读取配置文件
				//vector<vector<string>> str_ralation_Array;//输入设备关系集合
				//vector<vector<string>> str_definition_Array;//输入字段定义集合
				//ifstream inFile1("D:\\卫星星座运管系统\\遥测数据\\父子关系表.csv", ios::in);//配置文件1

				//string lineStr;//文件读取一行
				//getline(inFile1, lineStr);//跳过第一行
				//while (getline(inFile1, lineStr)) {
				//	cout << lineStr << endl;
				//	stringstream ss(lineStr);
				//	string str;
				//	vector<string> lineArray;
				//	// 按照逗号分隔  
				//	while (getline(ss, str, ','))
				//		lineArray.push_back(str);
				//	str_ralation_Array.push_back(lineArray);
				//}
				//inFile1.close();
				//ifstream inFile2("D:\\卫星星座运管系统\\遥测数据\\字段定义表.csv", ios::in);//配置文件2
				//getline(inFile2, lineStr);//跳过第一行
				//while (getline(inFile2, lineStr)) {
				//	cout << lineStr << endl;
				//	stringstream ss(lineStr);
				//	string str;
				//	vector<string> lineArray;
				//	// 按照逗号分隔  
				//	while (getline(ss, str, ','))
				//		lineArray.push_back(str);
				//	str_definition_Array.push_back(lineArray);
				//}
				//inFile2.close();
				//StringNumUtils utiles;
				//for (int i = 0; i < str_ralation_Array.size(); i++) {
				//	definitionMes mes;
				//	mes.name = str_ralation_Array[i][0];
				//	mes.p_name = str_ralation_Array[i][1];
				//	for (int j = 0; j < str_definition_Array.size(); j++) {
				//		if (str_definition_Array[j][7] == str_ralation_Array[i][0]) {
				//			field myfield;
				//			myfield.f_name = str_definition_Array[j][1];
				//			myfield.type = utiles.stringToNum<INT16>(str_definition_Array[j][2]);
				//			if (str_definition_Array[j][3] != "")myfield.min = utiles.stringToNum<double>(str_definition_Array[j][3]);
				//			if (str_definition_Array[j][4] != "")myfield.max = utiles.stringToNum<double>(str_definition_Array[j][4]);
				//			myfield.unit = str_definition_Array[j][5];
				//			myfield.display = utiles.stringToNum<bool>(str_definition_Array[j][6]);
				//			mes.f_list.push_back(myfield);
				//		}
				//	}
				//	D_MES_LIST.push_back(mes);
				//}
				////cout << "配置文件解析完毕！" << endl;
				////发送定义报文
				//for (int i = 0; i < D_MES_LIST.size(); i++) {
				//	//报文长度

				//	UINT32 message_len = 0;// = 117 + D_MES_LIST[i].f_list.size() * 67;//TODO
				//	BYTE message[M];
				//	BYTE* ptr = message;//message指针
				//	UINT16 mesage_id = 1000;//报文标识
				//	long long timestamp = Message::getSystemTime();//时间戳
				//	bool encryption = false;//加密标识
				//	char name[40];//设备名称
				//	strcpy_s(name, 40, D_MES_LIST[i].name.c_str());
				//	char parent_name[40];//父设备名称
				//	strcpy_s(parent_name, 40, D_MES_LIST[i].p_name.c_str());
				//	//包装message
				//	memcpy(ptr, &mesage_id, sizeof(UINT16));//设备标识
				//	ptr = ptr + sizeof(UINT16);
				//	memcpy(ptr, &message_len, sizeof(UINT32));//字段长度
				//	ptr = ptr + sizeof(UINT32);
				//	memcpy(ptr, &timestamp, sizeof(long long));//时间
				//	ptr = ptr + sizeof(long long);
				//	memcpy(ptr, &encryption, sizeof(bool));//加密
				//	ptr = ptr + sizeof(bool);
				//	memcpy(ptr, &taskNum, sizeof(UINT32));//任务号
				//	ptr = ptr + sizeof(UINT32);
				//	memcpy(ptr, name, 40);//设备名
				//	ptr = ptr + 40;
				//	memcpy(ptr, parent_name, 40);//父设备名
				//	ptr = ptr + 40;
				//	memcpy(ptr, satelliteId, 20);//卫星编号
				//	ptr = ptr + 20;
				//	for (int j = 0; j < D_MES_LIST[i].f_list.size(); j++) {
				//		char field_name[40];//字段名称
				//		UINT16 field_type;//字段类型编号
				//		double min = NULL;//最小值
				//		double max = NULL;//最大值
				//		char unit[8];//单位
				//		bool display;//显示标志
				//		strcpy_s(field_name, 40, D_MES_LIST[i].f_list[j].f_name.c_str());
				//		field_type = D_MES_LIST[i].f_list[j].type;
				//		min = D_MES_LIST[i].f_list[j].min;
				//		max = D_MES_LIST[i].f_list[j].max;
				//		strcpy_s(unit, 8, D_MES_LIST[i].f_list[j].unit.c_str());
				//		display = D_MES_LIST[i].f_list[j].display;
				//		memcpy(ptr, field_name, 40);
				//		ptr = ptr + 40;
				//		memcpy(ptr, &field_type, sizeof(UINT16));
				//		ptr = ptr + sizeof(UINT16);
				//		memcpy(ptr, &min, sizeof(double));
				//		ptr = ptr + sizeof(double);
				//		memcpy(ptr, &max, sizeof(double));
				//		ptr = ptr + sizeof(double);
				//		memcpy(ptr, unit, 8);
				//		ptr = ptr + 8;
				//		memcpy(ptr, &display, sizeof(bool));
				//		ptr = ptr + sizeof(bool);
				//	}
				//	Sleep(10);
				//	message_len = ptr - message;
				//	memcpy(message + 2, &message_len, sizeof(UINT32));
				//	const int bufSize = 66560;//发送包固定65k
				//	int returnSize = 0;
				//	char sendBuf[bufSize];//申请发送buf
				//	ZeroMemory(sendBuf, bufSize);//清空发送空间
				//	memcpy(sendBuf, message, message_len);
				//	if (socketer.sendMessage(sendBuf, bufSize) == -1) {//发送包固定65k

				//													   //发送失败释放资源跳出文件读写
				//		cout << "| 卫星遥测         | 定义报文发送失败" << endl;
				//		break;
				//	}


				//}
				//vector<vector<string>> str_data_Array;//输入数据文件集合
				//ifstream inFile("D:\\卫星星座运管系统\\遥测数据\\数据.csv", ios::in);
				////解析数据文件
				//while (getline(inFile, lineStr)) {
				//	stringstream ss(lineStr);
				//	string str;
				//	vector<string> lineArray;
				//	// 按照逗号分隔  
				//	while (getline(ss, str, ','))
				//		lineArray.push_back(str);
				//	//str_data_Array.push_back(lineArray);
				//	BYTE buf[M];//数据buf
				//	BYTE* bufPtr = buf;//数据报文指针
				//	BYTE* buf_len_Ptr = buf;
				//	UINT16 id = 2000;//报文标识
				//	UINT32 len = 0;//报文长度
				//	memcpy(bufPtr, &id, sizeof(UINT16));//报文标识
				//	bufPtr = bufPtr + sizeof(UINT16);
				//	len = len + sizeof(UINT16);
				//	memcpy(bufPtr, &len, sizeof(UINT32));//报文长度
				//	bufPtr = bufPtr + sizeof(UINT32);
				//	len = len + sizeof(UINT16);
				//	long long timestamp = utiles.stringToNum<long long>(lineArray[0]);//产生时间
				//	memcpy(bufPtr, &timestamp, sizeof(long long));
				//	len = len + sizeof(long long);
				//	bufPtr = bufPtr + sizeof(long long);
				//	bool flag = 0;//加密标识
				//	memcpy(bufPtr, &flag, sizeof(bool));
				//	bufPtr = bufPtr + sizeof(bool);
				//	len = len + sizeof(bool);
				//	memcpy(bufPtr, &taskNum, sizeof(UINT32));//任务编号
				//	bufPtr = bufPtr + sizeof(UINT32);
				//	len = len + sizeof(bool);
				//	char name[40];//设备名称
				//	strcpy_s(name, 40, lineArray[1].c_str());
				//	memcpy(bufPtr, name, 40);
				//	bufPtr = bufPtr + 40;
				//	len = len + 40;
				//	memcpy(bufPtr, satelliteId, 20);//卫星编号
				//	bufPtr = bufPtr + 20;
				//	long long tm = Message::getSystemTime();//采样时间
				//	memcpy(bufPtr, &tm, sizeof(long long));
				//	bufPtr = bufPtr + sizeof(long long);
				//	//通过设备名去定位该设备的字段信息
				//	for (int j = 0; j < D_MES_LIST.size(); j++) {
				//		if (D_MES_LIST[j].name == lineArray[1]) {//如果找到了该数据的定义报文
				//			for (int k = 0; k < D_MES_LIST[j].f_list.size(); k++) {//在其定义报文字段查找类型并写入Buffer
				//				if (D_MES_LIST[j].f_list[k].type == 1) {//16位整数
				//					INT16 data1 = utiles.stringToNum<INT16>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data1, sizeof(INT16));
				//					bufPtr = bufPtr + sizeof(INT16);
				//					len = len + sizeof(INT16);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 2) {//16位无符号整数
				//					UINT16 data2 = utiles.stringToNum<UINT16>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data2, sizeof(UINT16));
				//					bufPtr = bufPtr + sizeof(UINT16);
				//					len = len + sizeof(UINT16);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 3) {//32位整数
				//					INT32 data3 = utiles.stringToNum<INT32>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data3, sizeof(INT32));
				//					bufPtr = bufPtr + sizeof(INT32);
				//					len = len + sizeof(INT32);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 4) {//32位无符号整数
				//					UINT32 data4 = utiles.stringToNum<UINT32>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data4, sizeof(UINT32));
				//					bufPtr = bufPtr + sizeof(UINT32);
				//					len = len + sizeof(UINT32);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 5) {//64位整数
				//					INT64 data5 = utiles.stringToNum<INT64>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data5, sizeof(INT64));
				//					bufPtr = bufPtr + sizeof(INT64);
				//					len = len + sizeof(INT64);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 6) {//64位无符号整数
				//					UINT64 data6 = utiles.stringToNum<UINT64>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data6, sizeof(UINT64));
				//					bufPtr = bufPtr + sizeof(UINT64);
				//					len = len + sizeof(UINT64);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 7) {//单精度浮点数值
				//					float data7 = utiles.stringToNum<float>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data7, sizeof(float));
				//					bufPtr = bufPtr + sizeof(float);
				//					len = len + sizeof(float);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 8) {//双精度浮点数值
				//					double data8 = utiles.stringToNum<double>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data8, sizeof(double));
				//					bufPtr = bufPtr + sizeof(double);
				//					len = len + sizeof(double);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 9) {//字符类型
				//					char data9 = lineArray[k + 2].data()[0];
				//					memcpy(bufPtr, &data9, sizeof(char));
				//					bufPtr = bufPtr + sizeof(char);
				//					len = len + sizeof(char);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 10) {//短字符串类型
				//					char buf_data[15];//数据包
				//					strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
				//					int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
				//					memcpy(bufPtr, lineArray[k + 2].data(), size);
				//					bufPtr = bufPtr + 15;
				//					len = len + 15;
				//				}

				//				else if (D_MES_LIST[j].f_list[k].type == 11) {//长字符串类型
				//					char buf_data[255];//数据包
				//					strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
				//					int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
				//					memcpy(bufPtr, buf_data, size);
				//					bufPtr = bufPtr + 255;
				//					len = len + 255;
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 12) {//短字节数组255
				//					char buf_data[255];//数据包
				//					strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
				//					int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
				//					memcpy(bufPtr, buf_data, size);
				//					bufPtr = bufPtr + 255;
				//					len = len + 255;
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 13) {//中字节数组32K
				//					char buf_data[32 * K];//数据包
				//					strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
				//					int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
				//					memcpy(bufPtr, buf_data, size);
				//					bufPtr = bufPtr + 32 * K;
				//					len = len + 32 * K;
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 14) {//长字节数组64K
				//					char buf_data[60 * K];//数据包
				//					strcpy(buf_data, lineArray[k + 2].data());//将数据拷贝到数据包
				//					int size = lineArray[k + 2].length() + 1;//考虑到'/0'要加1
				//					memcpy(bufPtr, buf_data, size);
				//					int add = 60 * K;
				//					bufPtr = bufPtr + add;
				//					len = len + 60 * K;

				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 15) {//BOOL
				//					bool data = utiles.stringToNum<bool>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data, sizeof(bool));
				//					bufPtr = bufPtr + sizeof(bool);
				//					len = len + sizeof(bool);
				//				}
				//				else if (D_MES_LIST[j].f_list[k].type == 16) {//时间戳
				//					long long data16 = utiles.stringToNum<long long>(lineArray[k + 2]);
				//					memcpy(bufPtr, &data16, sizeof(long long));
				//					bufPtr = bufPtr + sizeof(long long);
				//					len = len + sizeof(long long);
				//				}
				//			}
				//		}
				//	}
				//	Sleep(10);//按照设置的速率发送
				//	len = bufPtr - buf_len_Ptr;
				//	memcpy(buf_len_Ptr + 2, &len, sizeof(UINT32));
				//	const int bufSize = 66560;//发送包固定65k
				//	int returnSize = 0;
				//	char* sendBuf = new char[bufSize];//申请发送buf
				//	ZeroMemory(sendBuf, bufSize);//清空发送空间
				//	memcpy(sendBuf, buf, len);
				//	if (socketer.sendMessage(sendBuf, bufSize) == -1) {//发送包固定65k

				//													   //发送失败释放资源跳出文件读写
				//		cout << "| 卫星遥测         | 数据报文发送失败" << endl;
				//		delete sendBuf;

				//		break;
				//	}
				//	cout << "#";
				//	
				//}

				//inFile.close();
				//断开TCP
				socketer.offSendServer();
				delete groundStationId;
				delete satelliteId;
			}
			

		}
		else {
			cout << "| 卫星遥测         | 连接数据库失败" << endl;
			cout << "| 卫星遥测错误信息 | " << mysql.errorNum << endl;
		}
		cout << "| 卫星遥测         | 任务结束" << endl;

	}
	return 0;
}

DWORD tele2(LPVOID lpParameter)
{
	return 0;
}
