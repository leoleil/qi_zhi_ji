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
		char* bufPtr = buf;//buf指针
		memcpy(bufPtr, &(messageId), sizeof(UINT16));//任务标志
		bufPtr = bufPtr + sizeof(UINT16);//移动指针
		memcpy(bufPtr, &messageLength, sizeof(UINT32));//报文长度
		bufPtr = bufPtr + sizeof(UINT32);//移动指针
		memcpy(bufPtr, &dateTime, sizeof(long long));//时间戳
		bufPtr = bufPtr + sizeof(long long);//移动指针
		memcpy(bufPtr, &encrypt, sizeof(bool));//加密标识
		bufPtr = bufPtr + sizeof(bool);//移动指针
		memcpy(bufPtr, &stateStartTime, sizeof(long long));//计划开始时间戳
		bufPtr = bufPtr + sizeof(long long);//移动指针
		memcpy(bufPtr, uavNo, 20);//无人机编号
		bufPtr = bufPtr + 20;//移动指针
		memcpy(bufPtr, &state, sizeof(UINT));//报文内容
		message_size = messageLength;

	}
}

void StateMessage::messageParse(char * buf)
{
	char* bufPtr = buf;//buf指针
	memcpy(&(messageId), bufPtr, sizeof(UINT16));//任务标志
	bufPtr = bufPtr + sizeof(UINT16);//移动指针
	memcpy(&messageLength, bufPtr, sizeof(UINT32));//报文长度
	bufPtr = bufPtr + sizeof(UINT32);//移动指针
	memcpy(&dateTime, bufPtr, sizeof(long long));//时间戳
	bufPtr = bufPtr + sizeof(long long);//移动指针
	memcpy(&encrypt, bufPtr, sizeof(bool));//加密标识
	bufPtr = bufPtr + sizeof(bool);//移动指针
	memcpy(&stateStartTime, bufPtr, sizeof(long long));//计划开始时间戳
	bufPtr = bufPtr + sizeof(long long);//移动指针
	memcpy(uavNo, bufPtr, 20);//无人机编号
	bufPtr = bufPtr + 20;//移动指针
	memcpy(&state, bufPtr, sizeof(UINT));
}
