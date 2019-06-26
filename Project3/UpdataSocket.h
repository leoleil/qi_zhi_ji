#pragma once
#include "Socket.h"
class UpdataSocket :
	public Socket
{
public:
	UpdataSocket();
	~UpdataSocket();
public:
	int createReceiveServer(const int port, std::vector<message_buf>& message);
};

