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
	cout << "| �������         | ��������" << endl;
	//��ʼ���׽��ֶ�̬��  
	if (WSAStartup(MAKEWORD(2, 2), &S_wsd) != 0)
	{
		cout << "WSAStartup failed!" << endl;
		return 1;
	}

	//�����׽���  
	sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sServer)
	{
		cout << "socket failed!" << endl;
		WSACleanup();//�ͷ��׽�����Դ;  
		return  -1;
	}

	//�������׽��ֵ�ַ   
	addrServ.sin_family = AF_INET;
	addrServ.sin_port = htons(port);
	addrServ.sin_addr.s_addr = INADDR_ANY;
	//���׽���  
	retVal = bind(sServer, (LPSOCKADDR)&addrServ, sizeof(SOCKADDR_IN));
	if (SOCKET_ERROR == retVal)
	{
		cout << "bind failed!" << endl;
		closesocket(sServer);   //�ر��׽���  
		WSACleanup();           //�ͷ��׽�����Դ;  
		return -1;
	}

	//��ʼ����   
	cout << "| �������         | listening" << endl;
	retVal = listen(sServer, 1);

	if (SOCKET_ERROR == retVal)
	{
		cout << "listen failed!" << endl;
		closesocket(sServer);   //�ر��׽���  
		WSACleanup();           //�ͷ��׽�����Դ;  
		return -1;
	}

	//���ܿͻ�������  
	sockaddr_in addrClient;
	int addrClientlen = sizeof(addrClient);
	sClient = accept(sServer, (sockaddr FAR*)&addrClient, &addrClientlen);
	if (INVALID_SOCKET == sClient)
	{
		cout << "accept failed!" << endl;
		closesocket(sServer);   //�ر��׽���  
		WSACleanup();           //�ͷ��׽�����Դ;  
		return -1;
	}

	cout << "| �������         | TCP���Ӵ���" << endl;
	while (true) {
		//���ݴ���
		const int data_len = 66560;//ÿ�ν���65K���ݰ�
		char data[66560]; //���ݰ�
		ZeroMemory(data, data_len);//�����ݰ��ռ���0
		char* data_ptr = data;//����ָ��
		int r_len = 0;
		while (1) {
			//���տͻ�������
			//���buffer
			ZeroMemory(buf, BUF_SIZE);

			//��ȡ����
			retVal = recv(sClient, buf, BUF_SIZE, 0);

			if (SOCKET_ERROR == retVal)
			{
				cout << "| �������         | ���ճ������" << endl;
				closesocket(sServer);   //�ر��׽���    
				closesocket(sClient);   //�ر��׽���
				return -1;
			}
			if (retVal == 0) {
				cout << "| �������         | ������϶Ͽ���������" << endl;
				closesocket(sServer);   //�ر��׽���    
				closesocket(sClient);   //�ر��׽���
				return -1;
			}
			memcpy(data_ptr, buf, retVal);
			r_len = r_len + retVal;

			data_ptr = data_ptr + retVal;
			if ((data_ptr - data) >= data_len) {
				break;//������յ������ݴ�����󴰿�����ѭ�������ݰ�64K��
			}

		}


		//����ȡ�������ݷ������ݿ���
		const char * SERVER = MYSQL_SERVER.data();//���ӵ����ݿ�ip
		const char * USERNAME = MYSQL_USERNAME.data();
		const char * PASSWORD = MYSQL_PASSWORD.data();
		const char DATABASE[20] = "qian_zhi_ji";
		const int PORT = 3306;
		MySQLInterface mysql;
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�) VALUES (now(),'�������','����TCP����');");
			char* ptr = data;
			UINT32 length = 0;
			for (int i = 0; i < data_len; i = i + length) {
				if (data[i] == NULL && data[i + 1] == NULL)break;
				//��ȡ���ĳ���
				memcpy(&length, ptr + i + 2, 4);

				//��ȡһ��buffer
				char val[65 * 1024];
				//�ڴ渴��
				memcpy(val, ptr + i, length);
				//��������д�����ݿ�
				AllocationMessage message;
				message.messageParse(val);
				//����ǳ�������
				if (message.getterMessageId() == 4020) {
					string sql = "SELECT * FROM ������Ϣ�� where ����״̬ = 1 and �������� = 2 and ������ = ";
					sql = sql + to_string(message.getterTaskNum()) + ";";
					vector<vector<string>> s;
					mysql.getDatafromDB(sql, s);
					if (s.size() == 0) {
						cout << "| �������         | ����ʧ�ܣ������Ϊ" << to_string(message.getterTaskNum()) << endl;
						mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'�������','����ʧ�ܣ������Ϊ" + to_string(message.getterTaskNum()) + "'," + to_string(message.getterTaskNum()) + ");");
						continue;
					}
					sql = "UPDATE `������Ϣ��` SET `����״̬` = 6,`ACK` = 1500 WHERE `������` = ";
					sql = sql + to_string(message.getterTaskNum()) + ";";
					mysql.writeDataToDB(sql);
					cout << "| �������         | �����ɹ��������Ϊ" << to_string(message.getterTaskNum()) << endl;
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'�������','�����ɹ��������Ϊ" + to_string(message.getterTaskNum()) + "'," + to_string(message.getterTaskNum()) + ");");
				}
				else
				{
					int size;
					char satrlliteId[20];
					message.getterSatelliteId(satrlliteId, size);
					string sql = "INSERT INTO `������Ϣ��`(`�������ʱ��`,`������`,`���˻����`,`��������`,`��עʱ��`,`����״̬`)VALUES(now(),";
					sql = sql + to_string(message.getterTaskNum()) + ",'" + satrlliteId + "'," + to_string(message.getterTaskType()) + ",from_unixtime(" + to_string(message.getterTaskStartTime()) + "),1);";
					mysql.writeDataToDB(sql);
					int message_date_len = 64 * 1024;
					char message_date[64 * 1024];
					message.getterMessage(message_date, message_date_len);
					sql = "update `������Ϣ��` set `����ָ��` = '";
					sql = sql + message_date + "' where `������` = " + to_string(message.getterTaskNum()) + ";";
					mysql.writeDataToDB(sql);
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'�������','����ɹ��������Ϊ" + to_string(message.getterTaskNum()) + "'," + to_string(message.getterTaskNum()) + ");");
					cout << "| �������         | �ɹ�" << endl;
				}
				mysql.closeMySQL();
			}
		}
		else {
			cout << "| �������         | �������ݿ�ʧ��" << endl;
			cout << "| ������������Ϣ | " << mysql.errorNum << endl;
		}

	}
	//�˳�  
	closesocket(sServer);   //�ر��׽���  
	closesocket(sClient);   //�ر��׽���  

	return 0;
}
