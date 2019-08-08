#include "ACKMessage.h"



ACKMessage::ACKMessage(UINT16 messageId, long long dateTime, bool encrypt, UINT32 taskNum, long long taskStartTime, UINT ACK) :
	Message(messageId, dateTime, encrypt), taskNum(taskNum), taskStartTime(taskStartTime), ACK(ACK)
{
	this->messageLength = this->messageLength + sizeof(UINT32) + sizeof(long long) + sizeof(UINT);
}

ACKMessage::ACKMessage()
{
}


ACKMessage::~ACKMessage()
{
}

UINT32 ACKMessage::getTaskNum()
{
	return this->taskNum;
}

void ACKMessage::setTaskNum(UINT32 taskNum)
{
	this->taskNum = taskNum;
}

long long ACKMessage::getTaskStartTime()
{
	return this->taskStartTime;
}

void ACKMessage::setTaskStartTime(long long taskStartTime)
{
	this->taskStartTime = taskStartTime;
}


UINT ACKMessage::getACK()
{
	return this->ACK;
}

void ACKMessage::setACK(UINT ACK)
{
	this->ACK = ACK;
}

void ACKMessage::createMessage(char * buf, int & message_size, int buf_size)
{
	if (buf_size >= messageLength) {
		char* bufPtr = buf;//bufָ��
		memcpy(bufPtr, &(messageId), sizeof(UINT16));//�����־
		bufPtr = bufPtr + sizeof(UINT16);//�ƶ�ָ��
		memcpy(bufPtr, &messageLength, sizeof(UINT32));//���ĳ���
		bufPtr = bufPtr + sizeof(UINT32);//�ƶ�ָ��
		memcpy(bufPtr, &dateTime, sizeof(long long));//ʱ���
		bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
		memcpy(bufPtr, &encrypt, sizeof(bool));//���ܱ�ʶ
		bufPtr = bufPtr + sizeof(bool);//�ƶ�ָ��
		memcpy(bufPtr, &taskNum, sizeof(UINT32));//������
		bufPtr = bufPtr + sizeof(UINT32);//�ƶ�ָ��
		memcpy(bufPtr, &taskStartTime, sizeof(long long));//�ƻ���ʼʱ���
		bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
		memcpy(bufPtr, &ACK, sizeof(UINT));//���Ǳ��
		message_size = messageLength;

	}

}

void ACKMessage::messageParse(char * buf)
{
	char* bufPtr = buf;//bufָ��
	memcpy(&(messageId), bufPtr, sizeof(UINT16));//�����־
	bufPtr = bufPtr + sizeof(UINT16);//�ƶ�ָ��
	memcpy(&messageLength, bufPtr, sizeof(UINT32));//���ĳ���
	bufPtr = bufPtr + sizeof(UINT32);//�ƶ�ָ��
	memcpy(&dateTime, bufPtr, sizeof(long long));//ʱ���
	bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
	memcpy(&encrypt, bufPtr, sizeof(bool));//���ܱ�ʶ
	bufPtr = bufPtr + sizeof(bool);//�ƶ�ָ��
	memcpy(&taskNum, bufPtr, sizeof(UINT32));//������
	bufPtr = bufPtr + sizeof(UINT32);//�ƶ�ָ��
	memcpy(&taskStartTime, bufPtr, sizeof(long long));//�ƻ���ʼʱ���
	bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
	memcpy(&ACK, bufPtr, sizeof(UINT));
}

