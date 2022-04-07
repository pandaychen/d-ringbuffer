#ifndef __NODE_H_
#define __NODE_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>

#define BUFFER_RESERVE_LENGTH 8 //补齐长度
#define SHM_KEY_A 10001
#define SHM_SIZE_A 1024 * 1024

enum SHM_RET
{
	SHM_ERROR = -1,
	CREATE_SUCC = 0,
	ATTACH_SUCC = 1,
};

struct NodeDataHead
{
	int iSize;
	int iRead;
	int iWrite;
	int iOffset;
};

struct NodeDataUnit
{
	int iLen; //可变长
	char *pData;
};

#endif