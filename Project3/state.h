#pragma once

#include <iostream>  
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "winsock2.h" 
#include <time.h>
#include <Windows.h>
#include "MySQLInterface.h"
#include "Message.h"
#include "StateMessage.h"
#include "StringNumUtils.h"
#include "Socket.h"

using namespace std;

extern string MYSQL_SERVER;
extern string MYSQL_USERNAME;
extern string MYSQL_PASSWORD;
/*״̬ģ��*/
//״̬����߳����
DWORD WINAPI state(LPVOID lpParameter);

