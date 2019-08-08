#include "state.h"

DWORD state(LPVOID lpParameter)
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
		MySQLInterface mysql;//�������ݿ����Ӷ���

							 //�������ݿ�
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//�������л�ȡ��������
			string selectSql = "select ����,���˻����,���˻�״̬,unix_timestamp(״̬��Чʱ��) from ״̬��¼��;";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//������Ĭ5��������ѯ
			}
			//��ѯ������
			for (int i = 0, len = dataSet.size(); i < len; i++) {
			
				StringNumUtils util;//�ַ�ת���ֹ���

				long long dateTime = Message::getSystemTime();//��ȡ��ǰʱ���
				bool encrypt = false;//�Ƿ����
				UINT state = util.stringToNum<UINT>(dataSet[i][2]);//״̬
				long long stateStartTime = util.stringToNum<long long>(dataSet[i][3]);//״̬��Чʱ��
				char* uavNo = new char[20];//���˻����
				strcpy_s(uavNo, dataSet[i][1].size() + 1, dataSet[i][1].c_str());

				//���ҵ���վip��ַ���ͱ���
				string groundStationSql = "select IP��ַ from ��������Ϣ��";
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					continue;//û���ҵ�ip��ַ
				}

				//����������
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//��ȡ����ַ
													 //����TCP����
				if (!socketer.createSendServer(ip, 5002, 0)) {
					//�������ɹ��ͷ���Դ
					continue;
				}
				const int bufSize = 66560;//���Ͱ��̶�65k
				int returnSize = 0;
				char* sendBuf = new char[bufSize];//���뷢��buf
				ZeroMemory(sendBuf, bufSize);//��շ��Ϳռ�
				StateMessage message(3010, Message::getSystemTime(), false, uavNo, stateStartTime, state);
				delete uavNo;//�ͷ���Դ
				message.createMessage(sendBuf, returnSize, bufSize);
				if (socketer.sendMessage(sendBuf, bufSize) == -1) {
					//����ʧ��
					cout << "| ״̬����         | ����ʧ��" << endl;
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'״̬����','����ʧ��'," + dataSet[i][0] + ");");
				}
				string ackSql = "delete from ״̬��¼�� where ���� = " + dataSet[i][0];
				
				cout << "| ״̬����         | ���ͳɹ�" << endl;
				mysql.writeDataToDB(ackSql);
				delete sendBuf;
				//�Ͽ�TCP
				socketer.offSendServer();
			}
			mysql.closeMySQL();

		}
		else {
			cout << "| ״̬����         | �������ݿ�ʧ��" << endl;
			cout << "| ״̬���ʹ�����Ϣ | " << mysql.errorNum << endl;
		}
		cout << "| ״̬����         | �������" << endl;

	}
	return 0;
}
