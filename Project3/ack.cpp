#include "ack.h"

DWORD ack(LPVOID lpParameter)
{
	//���ݿ����ӹؼ���
	const char * SERVER = MYSQL_SERVER.data();
	const char * USERNAME = MYSQL_USERNAME.data();
	const char * PASSWORD = MYSQL_PASSWORD.data();
	const char DATABASE[20] = "qian_zhi_ji";
	const int PORT = 3306;
	while (1) {
		//5�������ݿ��������Ϣ��
		Sleep(5000);
		//cout << "| ACK ����         | ������ݿ�����..." << endl;
		MySQLInterface mysql;//�������ݿ����Ӷ���

							 //�������ݿ�
		if (mysql.connectMySQL(SERVER, USERNAME, PASSWORD, DATABASE, PORT)) {
			//�������л�ȡ��������
			string selectSql = "select ������,unix_timestamp(��עʱ��),����״̬ from ������Ϣ�� where ����״̬ = 2 || ����״̬ = 4 || ����״̬ = 6";
			vector<vector<string>> dataSet;
			mysql.getDatafromDB(selectSql, dataSet);
			if (dataSet.size() == 0) {
				continue;//������Ĭ5��������ѯ
			}
			//��ѯ������
			for (int i = 0, len = dataSet.size(); i < len; i++) {
				char* messageDate;
				int messageDataSize = 0;
				//if (!dataSet[i][1]._Equal("111")) {

				//	continue;//�����ȴ�
				//}
				StringNumUtils util;//�ַ�ת���ֹ���

				long long dateTime = Message::getSystemTime();//��ȡ��ǰʱ���
				bool encrypt = false;//�Ƿ����
				UINT32 taskNum = util.stringToNum<UINT32>(dataSet[i][0]);//������
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
				long long taskStartTime = util.stringToNum<long long>(dataSet[i][1]);//��עʱ��

				//���ҵ���վip��ַ���ͱ���
				string groundStationSql = "select IP��ַ from ��������Ϣ��;";
				vector<vector<string>> ipSet;
				mysql.getDatafromDB(groundStationSql, ipSet);
				if (ipSet.size() == 0) {
					continue;//û���ҵ�ip��ַ
				}

				//����������
				Socket socketer;
				const char* ip = ipSet[0][0].c_str();//��ȡ����ַ
				//����TCP����
				if (!socketer.createSendServer(ip, 5000, 0)) {
					//�������ɹ��ͷ���Դ
					continue;
				}
				const int bufSize = 66560;//���Ͱ��̶�65k
				int returnSize = 0;
				char* sendBuf = new char[bufSize];//���뷢��buf
				ZeroMemory(sendBuf, bufSize);//��շ��Ϳռ�
				ACKMessage message(9000, Message::getSystemTime(), false, taskNum, taskStartTime, ACK);
				message.createMessage(sendBuf, returnSize, bufSize);
				if (socketer.sendMessage(sendBuf, bufSize) == -1) {
					//����ʧ��
					cout << "| ACK ����         | ����ʧ��" << endl;
					mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'ACK ����','����ʧ��',"+ dataSet[i][0] +");");
				}
				string ackSql = "";
				if (dataSet[i][2]._Equal("2")) {
					ackSql = "update ������Ϣ�� set ����״̬ = 3 where ������ = " + dataSet[i][0];
				}
				else if (dataSet[i][2]._Equal("4")) {
					ackSql = "update ������Ϣ�� set ����״̬ = 5 where ������ = " + dataSet[i][0];
				}
				else {
					ackSql = "update ������Ϣ�� set ����״̬ = 7 where ������ = " + dataSet[i][0];
				}
				cout << "| ACK ����         | ���ͳɹ�" << endl;
				mysql.writeDataToDB("INSERT INTO ϵͳ��־��(ʱ��,ģ��,�¼�,������) VALUES (now(),'ACK ����','���ͳɹ�'," + dataSet[i][0] + ");");
				mysql.writeDataToDB(ackSql);

				delete sendBuf;
				//�Ͽ�TCP
				socketer.offSendServer();
			}
			mysql.closeMySQL();

		}
		else {
			cout << "| ACK ����         | �������ݿ�ʧ��" << endl;
			cout << "| ACK ���ʹ�����Ϣ | " << mysql.errorNum << endl;
		}
		cout << "| ACK ����         | �������" << endl;

	}
	return 0;
}
