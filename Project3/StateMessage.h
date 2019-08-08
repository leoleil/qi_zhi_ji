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
	char uavNo[20];//���˻����
	long long stateStartTime;//״̬��Чʱ��
	UINT state;
public:
	void getUavNo(char* uavNo);
	long long getStateStartTime();
	UINT getState();
	void createMessage(char* buf, int & message_size, int buf_size);
	void messageParse(char* buf);
};

