#include "assign.h"

DWORD WINAPI assign(LPVOID lpParameter) {
	
	while (1) {
		AssignmentSocket service;//创建接收任务服务
		service.createReceiveServer(4998, MESSAGES);
	}
	return 0;
}

