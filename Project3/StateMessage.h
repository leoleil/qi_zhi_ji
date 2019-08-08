#pragma once
#include <winsock2.h> 
#include "Message.h"
class StateMessage :
	public Message
{
public:
	StateMessage(UINT16 messageId, long long dateTime, bool encrypt, char* uavNo, long long stateStartTime, UINT state);
	StateMessage();
	~StateMessage();
private:
	char uavNo[20];//无人机编号
	long long stateStartTime;//状态生效时间
	UINT state;
public:
	void getUavNo(char* uavNo);
	long long getStateStartTime();
	UINT getState();
	void createMessage(char* buf, int & message_size, int buf_size);
	void messageParse(char* buf);
};

