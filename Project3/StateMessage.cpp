#include "StateMessage.h"



StateMessage::StateMessage(UINT16 messageId, long long dateTime, bool encrypt, char * uavNo, long long stateStartTime, UINT state) :
	Message(messageId, dateTime, encrypt), stateStartTime(stateStartTime),state(state)
{
	strcpy_s(this->uavNo, 20, uavNo);
	this->messageLength = this->messageLength + sizeof(long long) + 20 + sizeof(UINT);
}

StateMessage::StateMessage()
{
}


StateMessage::~StateMessage()
{
}

void StateMessage::getUavNo(char* uavNo)
{
	strcpy_s(uavNo, 20, this->uavNo);
}

long long StateMessage::getStateStartTime()
{
	return this->stateStartTime;
}

UINT StateMessage::getState()
{
	return this->state;
}

void StateMessage::createMessage(char * buf, int & message_size, int buf_size)
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
		memcpy(bufPtr, &stateStartTime, sizeof(long long));//�ƻ���ʼʱ���
		bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
		memcpy(bufPtr, uavNo, 20);//���˻����
		bufPtr = bufPtr + 20;//�ƶ�ָ��
		memcpy(bufPtr, &state, sizeof(UINT));//��������
		message_size = messageLength;

	}
}

void StateMessage::messageParse(char * buf)
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
	memcpy(&stateStartTime, bufPtr, sizeof(long long));//�ƻ���ʼʱ���
	bufPtr = bufPtr + sizeof(long long);//�ƶ�ָ��
	memcpy(uavNo, bufPtr, 20);//���˻����
	bufPtr = bufPtr + 20;//�ƶ�ָ��
	memcpy(&state, bufPtr, sizeof(UINT));
}
