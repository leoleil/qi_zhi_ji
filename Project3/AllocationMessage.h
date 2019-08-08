#pragma once
#include <winsock2.h> 
#include "Message.h"
class AllocationMessage :
	public Message
{
public:
	AllocationMessage(UINT16 messageId, long long dateTime, bool encrypt, UINT32 taskNum, UINT16 taskType, long long taskStartTime, char* satelliteId);
	AllocationMessage();
	~AllocationMessage();
private:
	UINT32 taskNum;//������
	UINT16 taskType;//��������
	long long taskStartTime;//��עʱ��
	char satelliteId[20];//���˻����
	char save[4];//�����ֽ�
	int messageSize = 0;//���ĳ���
	char* message;//��������
public:
	UINT32 getterTaskNum();
	void setterTaskNum(UINT32 taskNum);
	UINT16 getterTaskType();
	void setterTaskType(UINT16 taskType);
	long long getterTaskStartTime();
	void setterTaskStartTime(long long taskStartTime);
	void getterSatelliteId(char* satelliteId, int & size);
	void setterSatelliteId(char* satelliteId, int size);
	int getterMessageSize();
	void setterMessageSize(int messageSize);
	void getterMessage(char* message,int & size);
	void setterMessage(char* message,int size);
public:
	void createMessage(char* buf, int & message_size, int buf_size);
	void messageParse(char* buf);

};

