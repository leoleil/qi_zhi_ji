#pragma once
#include "Socket.h"
class AssignmentSocket :
	public Socket
{
public:
	AssignmentSocket();
	~AssignmentSocket();
public:
	int createReceiveServer(const int port, std::vector<message_buf>& message);

};

