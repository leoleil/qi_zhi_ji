#include "assignment_data_download.h"

DWORD downdata(LPVOID lpParameter)
{
	//���ݿ����ӹؼ���
	const char * SERVER = MYSQL_SERVER.data();
	const char * USERNAME = MYSQL_USERNAME.data();
	const char * PASSWORD = MYSQL_PASSWORD.data();
	const char DATABASE[20] = "qian_zhi_ji";
	const int PORT = 3306;
	while (1) {
		//5�������ݿ����������
		Sleep(5000);
		//cout << "| ��������         | ������ݿ�����..." << endl;
		MySQLInterface mysql;//�������ݿ����Ӷ���

		//�������ݿ�
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//�������л�ȡ��������
			//Ѱ�ҷַ���־Ϊ2�����ݷַ���־Ϊ0������
			string selectSql = "select ����,������,���˻���� from �������и��±�";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//������Ĭ5��������ѯ
			}
			//��ѯ������
			for (int i = 0, len = dataSet.size(); i < len; i++) {
				char* messageDate;
				int messageDataSize = 0;
				//������������
				StringNumUtils util;//�ַ�ת���ֹ���

				long long dateTime = Message::getSystemTime();//��ȡ��ǰʱ���
				bool encrypt = false;//�Ƿ����
				UINT32 taskNum = util.stringToNum<UINT32>(dataSet[i][1]);//������
				string ackSql = "";
				char* uavId = new char[20];//���˻����
				strcpy_s(uavId, dataSet[i][2].size() + 1, dataSet[i][2].c_str());

				//���ҵ���վip��ַ���ͱ���
				string groundStationSql = "select IP��ַ from ��������Ϣ��";
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					delete uavId;
					continue;//û���ҵ�ip��ַ
				}

				//����������
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//��ȡ����ַ
													 //����TCP����
				if (!socketer.createSendServer(ip, 4997, 0)) {
					//�������ɹ��ͷ���Դ
					delete uavId;
					continue;
				}
				MySQLInterface diskMysql;
				if (!diskMysql.connectMySQL(SERVER, USERNAME, PASSWORD, "disk", PORT)) {
					
					cout << "| ��������         | �������ݿ�ʧ��" << endl;
					cout << "| �������д�����Ϣ | " << diskMysql.errorNum << endl;
					break;
				}
				vector<vector<string>> disk;
				diskMysql.getDatafromDB("SELECT * FROM disk.����λ��;", disk);
				if (disk.size() == 0) {
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'��������','����λ��δ֪',"+ dataSet[i][0] +");");
					cout << "| ��������         | ����λ��δ֪���������ݿ����á�" << endl;
					mysql.writeDataToDB(ackSql);
					//�������ɹ��ͷ���Դ
					continue;
				}
				string path = disk[0][1];
				path = path + "\\���д�������\\" + dataSet[i][2];
				vector<string> files;//Ҫ�ϴ����ļ�
				// �ļ����
				//long hFile = 0;  //win7
				intptr_t hFile = 0;   //win10
				// �ļ���Ϣ
				struct _finddata_t fileinfo;
				string p;
				if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
					do {
						if ((strcmp(fileinfo.name, ".") != 0) && (strcmp(fileinfo.name, "..") != 0)) {
							// �����ļ���ȫ·��
							string s = "";
							files.push_back(s.append(fileinfo.name));
						}
						
 					} while (_findnext(hFile, &fileinfo) == 0); //Ѱ����һ�����ɹ�����0������-1

					_findclose(hFile);
				}
				if (files.size() == 0) {
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'��������','�������ļ�'," + dataSet[i][0] + ");");
					cout << "| ��������         | ";
					cout << path << " �������ļ�" << endl;
					//�������ɹ��ͷ���Դ
					delete uavId;
					continue;
				}
				int pos = files[0].find_last_of('.');
				string fileName(files[0].substr(0,pos));//�ļ���
				string expandName(files[0].substr(pos));//��չ��
				string file = path.append("\\").append(files[0]);
				ifstream fileIs(file, ios::binary | ios::in);
				if (!fileIs.is_open()) {
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'��������','�����ļ��޷���'," + dataSet[i][0] + ");");
					cout << "| ��������         | ";
					cout << file << " �޷���" << endl;
					//�������ɹ��ͷ���Դ
					delete uavId;
					continue;
				}
				cout << "| ��������         | ";
				//��ȡ�ļ�
				while (!fileIs.eof()) {
					int bufLen = 1024 * 64;//�������64K
					char* fileDataBuf = new char[bufLen];//64K
					fileIs.read(fileDataBuf, bufLen);
					bufLen = fileIs.gcount();//��ȡʵ�ʶ�ȡ���ݴ�С

					char* up_file_name = new char[32];//�ļ���
					strcpy_s(up_file_name, fileName.size() + 1, fileName.c_str());
					char* up_expand_name = new char[8];//��չ��
					strcpy_s(up_expand_name, expandName.size() + 1, expandName.c_str());
					bool endFlag = fileIs.eof();//�ļ�β�ж�
												//�����������а�
					DownMessage downMessage(3020, dateTime, encrypt, taskNum, up_file_name, up_expand_name, endFlag);
					downMessage.setterData(fileDataBuf, bufLen);//��������

					const int bufSize = 66560;//���Ͱ��̶�65k
					int returnSize = 0;
					char* sendBuf = new char[bufSize];//���뷢��buf
					ZeroMemory(sendBuf, bufSize);//��շ��Ϳռ�
					downMessage.createMessage(sendBuf, returnSize, bufSize);//���������ֽڰ�
					Sleep(10);
					if (socketer.sendMessage(sendBuf, bufSize) == -1) {//���Ͱ��̶�65k
						//����ʧ���ͷ���Դ�����ļ���д
						mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'��������','����ʧ�ܣ��Ͽ�����'," + dataSet[i][0] + ");");
						cout << "| ��������         | ����ʧ�ܣ��Ͽ�����" << endl;
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
						cout << "| ��������         | " << dataSet[i][0] << "���������гɹ�" << endl;
						mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'��������','���ͳɹ��Ͽ�����'," + dataSet[i][0] + ");");
						//�޸����ݿ�ַ���־
						ackSql = "delete from �������и��±� where ���� = " + dataSet[i][0];
						mysql.writeDataToDB(ackSql);
						
					}

					delete sendBuf;
					delete up_expand_name;
					delete up_file_name;
					delete fileDataBuf;

				}
				//�Ͽ�TCP
				socketer.offSendServer();
				fileIs.close();
				delete uavId;
			}
			mysql.closeMySQL();

		}
		else {
			cout << "| ��������         | �������ݿ�ʧ��" << endl;
			cout << "| �������д�����Ϣ | " << mysql.errorNum << endl;
		}
		cout << "| ��������         | �������" << endl;

	}
	return 0;
}
