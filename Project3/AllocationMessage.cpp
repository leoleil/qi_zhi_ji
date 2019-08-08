#include "AllocationMessage.h"



AllocationMessage::AllocationMessage(UINT16 messageId, long long dateTime, bool encrypt, UINT32 taskNum, UINT16 taskType, long long taskStartTime, char* satelliteId):
	Message(messageId, dateTime, encrypt), taskNum(taskNum), taskType(taskType), taskStartTime(taskStartTime)
{
	strcpy_s(this->satelliteId, 20, satelliteId);
	this->messageLength = this->messageLength + sizeof(UINT32) + sizeof(UINT16) + sizeof(long long) + 20;
}
AllocationMessage::AllocationMessage() {

}

AllocationMessage::~AllocationMessage()
{
}

UINT32 AllocationMessage::getterTaskNum()
{
	return taskNum;
}

void AllocationMessage::setterTaskNum(UINT32 taskNum)
{
	this->taskNum = taskNum;
}

UINT16 AllocationMessage::getterTaskType()
{
	return taskType;
}

void AllocationMessage::setterTaskType(UINT16 taskType)
{
	this->taskType = taskType;
}

long long AllocationMessage::getterTaskStartTime()
{
	return taskStartTime;
}

void AllocationMessage::setterTaskStartTime(long long taskStartTime)
{
	this->taskStartTime = taskStartTime;
}

void AllocationMessage::getterSatelliteId(char* satelliteId, int & size)
{
	strcpy_s(satelliteId, 20, this->satelliteId);
	size = 20;
}

void AllocationMessage::setterSatelliteId(char* satelliteId, int size)
{
	if (size <= 20) {
		strcpy_s(this->satelliteId, 20, satelliteId);
	}
}

int AllocationMessage::getterMessageSize()
{
	return messageSize;
}

void AllocationMessage::setterMessageSize(int messageSize)
{
	this->messageSize = messageSize;
}

void AllocationMessage::getterMessage(char * message, int & size)
{
	strcpy_s(message, this->messageSize, this->message);
	size = this->messageSize;
}

void AllocationMessage::setterMessage(char * message, int size)
{
	this->message= new char[size];
	strcpy_s(this->message, size, message);
	this->messageSize = size;
	this->messageLength = this->messageLength + size;
}

void AllocationMessage::createMessage(char * buf, int & message_size, int buf_size)
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
		memcpy(bufPtr, &taskType, sizeof(UINT16));//��������
		bufPtr = bufPtr + sizeof(UINT16);//�ƶ�ָ��
		memcpy(bufPtr, &taskStartTime, sizeof(long long));//�ƻ���ʼʱ���
		bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
		memcpy(bufPtr, satelliteId, 20);//���˻����
		bufPtr = bufPtr + 20;//�ƶ�ָ��
		memcpy(bufPtr, message, messageSize);//��������
		message_size = messageLength;
		
	}
	
}

void AllocationMessage::messageParse(char * buf)
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
	memcpy(&taskType, bufPtr, sizeof(UINT16));//��������
	bufPtr = bufPtr + sizeof(UINT16);//�ƶ�ָ��
	memcpy(&taskStartTime, bufPtr, sizeof(long long));//�ƻ���ʼʱ���
	bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
	memcpy(satelliteId, bufPtr, 20);//���˻����
	bufPtr = bufPtr + 20;//�ƶ�ָ��
	messageSize = messageLength - (bufPtr - buf);
	message = new char[messageSize];//Ϊmessage�ṩ�ڴ�
	memcpy(message, bufPtr, messageSize);
}
