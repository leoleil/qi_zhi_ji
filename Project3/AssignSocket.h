#pragma once
#include "Socket.h"

class AssignSocket :
	public Socket
{
public:
	AssignSocket();
	~AssignSocket();

public:
	int createReceiveServer(const int port, std::vector<message_buf>& message);
};

