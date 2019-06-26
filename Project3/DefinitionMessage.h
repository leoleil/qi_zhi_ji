#pragma once
#include "Message.h"

class DefinitionMessage :
	public Message
{
public:
	DefinitionMessage();
	~DefinitionMessage();
public:
	UINT32 taskNum;//任务编号
	char satelliteId[20];//卫星编号
	char equipmentName[20];//设备名称
	char equipmentNameP[20];//父设备名称

};

