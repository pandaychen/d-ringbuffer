#ifndef _SHM_BUFF_QUEUE_H_
#define _SHM_BUFF_QUEUE_H_

// ringqueue的实现

#include "shm.h"
#include "node.h"

enum INVALID_TYPE
{
	WRITE_INDEX_FULL = -2,
	READ_INDEX_INVALID = -3,
	DATA_UNIT_INDEX_INVALID = -4,
	WRITE_INDEX_INVALID = -5,
};

class ShmRingQueue
{
public:
	ShmRingQueue(char *pShmBuff);
	~ShmRingQueue();

	// 从ShmRingQueue中读数据
	int GetDataUnit(char *pOut, int *psOutLen);

	// 向ShmRingQueue中写数据
	int PutDataUnit(const char *pIn, int sInLen);

	void PrintInfo();

private:
	ShmRingQueue() {}
	int GetLeftSize();
	int GetUsedSize();
	bool IsFull();
	NodeDataHead *m_pDataHead;
	char *m_pBuff; //指向首地址
};
#endif
