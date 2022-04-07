#include "ringqueue.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// 初始化ringqueue的成员
ShmRingQueue::ShmRingQueue(char *pShmBuff)
{
	m_pDataHead = (NodeDataHead *)pShmBuff;
	m_pBuff = pShmBuff + sizeof(NodeDataHead);
}

//
ShmRingQueue::~ShmRingQueue()
{
	m_pBuff = NULL;
	m_pDataHead = NULL;
}

void ShmRingQueue::PrintInfo()
{
	printf("DataHead:\n\tiSize:%u\n\tiBegin:%u\n\tiEnd:%u\n\tiOffset:%u\n", m_pDataHead->iSize, m_pDataHead->iWrite, m_pDataHead->iRead, m_pDataHead->iOffset);
}

// 获取当前ringqueue的剩余可用大小
int ShmRingQueue::GetLeftSize()
{
	int iRetSize = 0;
	int iWritePos = -1;
	int iReadPos = -1;

	iWritePos = m_pDataHead->iWrite;
	iReadPos = m_pDataHead->iRead;

	//首尾相等，无数据
	if (iReadPos == iWritePos)
	{
		iRetSize = m_pDataHead->iSize; //
	}
	//首大于尾，一般情况，iWritePos始终在"前"
	else if (iWritePos > iReadPos)
	{
		iRetSize = iWritePos - iReadPos;
	}
	//首小于尾，分开计算
	else
	{
		iRetSize = m_pDataHead->iSize - iWritePos + iReadPos;
	}

	//注意：最大长度减去预留部分长度，保证首尾不会相接
	iRetSize -= BUFFER_RESERVE_LENGTH;

	return iRetSize;
}

// 判断当前ringueue是否已满（包含了减掉BUFFER_RESERVE_LENGTH的）
bool ShmRingQueue::IsFull()
{
	int iLeftSize = 0;
	iLeftSize = GetLeftSize();
	if (iLeftSize > 0)
	{
		return false;
	}
	else
	{
		return false;
	}
}

// 是否需要加上BUFFER_RESERVE_LENGTH
int ShmRingQueue::GetUsedSize()
{
	int iLeftSize = GetLeftSize();
	if (iLeftSize > 0)
	{
		return m_pDataHead->iSize - iLeftSize;
	}
	else
	{
		return m_pDataHead->iSize;
	}
}

int ShmRingQueue::GetDataUnit(char *pOut, int *pnOutLen)
{
	int iLeftSize = 0;
	int iReadPos = -1;
	int iWritePos = -1;
	char *pbyCodeBuf = m_pBuff;
	char *pTempSrc = NULL;
	char *pTempDst = NULL;

	//参数判断
	if ((NULL == pOut) || (NULL == pnOutLen))
	{
		return -1;
	}

	if (m_pDataHead->iOffset <= 0 || m_pDataHead->iSize <= 0)
	{
		return -1;
	}

	//取读写指针
	iReadPos = m_pDataHead->iRead; //
	iWritePos = m_pDataHead->iWrite;

	//无数据
	if (iReadPos == iWritePos)
	{
		*pnOutLen = 0;
		return 0;
	}

	//剩余缓冲大小,小于包长度字节数,错误返回
	iLeftSize = GetLeftSize();
	if (iLeftSize < sizeof(int))
	{
		//异常情况，重置首尾，返回错误
		*pnOutLen = 0;
		m_pDataHead->iRead = 0;
		m_pDataHead->iWrite = 0;
		return READ_INDEX_INVALID;
	}

	// copy data
	pTempDst = (char *)pnOutLen;
	pTempSrc = (char *)&pbyCodeBuf[0];

	//包长度编码
	for (int i = 0; i < sizeof(int); i++)
	{
		pTempDst[i] = pTempSrc[iReadPos];
		iReadPos = (iReadPos + 1) % m_pDataHead->iSize;
	}

	//数据包长度非法
	if (((*pnOutLen) > GetUsedSize()) || (*pnOutLen < 0))
	{
		*pnOutLen = 0;
		m_pDataHead->iRead = 0;
		m_pDataHead->iWrite = 0;
		return DATA_UNIT_INDEX_INVALID;
	}

	pTempDst = pOut;

	//首小于尾，未跨越终点
	if (iReadPos < iWritePos)
	{
		memcpy((void *)pTempDst, (const void *)&pTempSrc[iReadPos], (size_t)(*pnOutLen));
	}
	else
	{
		//首大于尾且出现分段，则需要分段拷贝
		int iRightLeftSize = m_pDataHead->iSize - iReadPos; //查看当前要读取的数据是否被分段了
		if (iRightLeftSize < *pnOutLen)
		{
			//分段拷贝
			memcpy((void *)pTempDst, (const void *)&pTempSrc[iReadPos], iRightLeftSize);
			pTempDst += iRightLeftSize;
			memcpy((void *)pTempDst, (const void *)&pTempSrc[0], (size_t)(*pnOutLen - iRightLeftSize));
		}
		//否则，直接拷贝（临界情况），待拷贝的数据长度没有跨越分段
		else
		{
			memcpy((void *)pTempDst, (const void *)&pTempSrc[iReadPos], (size_t)(*pnOutLen));
		}
	}

	//变更读指针
	iReadPos = (iReadPos + (*pnOutLen)) % m_pDataHead->iSize;
	//更新iRead
	m_pDataHead->iRead = iReadPos;

	return iReadPos;
}

//写入数据
int ShmRingQueue::PutDataUnit(const char *pIn, int nInLen)
{
	int iLeftSize = 0;
	int iRead = -1;
	int iWrite = -1;

	//参数判断
	if ((NULL == pIn) || (nInLen <= 0))
	{
		return -1;
	}

	if (m_pDataHead->iOffset <= 0 || m_pDataHead->iSize <= 0)
	{
		return -1;
	}

	//首先判断是已满
	if (IsFull())
	{
		return WRITE_INDEX_FULL;
	}

	//取首、尾
	iRead = m_pDataHead->iRead;
	iWrite = m_pDataHead->iWrite;

	//缓冲区异常判断处理
	if (iRead < 0 || iRead >= m_pDataHead->iSize || iWrite < 0 || iWrite >= m_pDataHead->iSize)
	{
		//非法的index，重置
		m_pDataHead->iWrite = 0;
		m_pDataHead->iRead = 0;
		return WRITE_INDEX_INVALID;
	}

	//剩余缓冲大小小于新来的数据,溢出了,返回错误
	iLeftSize = GetLeftSize();
	if ((int)(nInLen + sizeof(int)) > iLeftSize)
	{
		//空闲不够，无法写入
		return WRITE_INDEX_FULL;
	}

	//数据首指针
	char *pbyCodeBuf = m_pBuff;

	char *pTempSrc = NULL;
	char *pTempDst = NULL;

	pTempDst = &pbyCodeBuf[0];
	pTempSrc = (char *)&nInLen;

	//包的长度编码
	for (int i = 0; i < sizeof(nInLen); i++)
	{
		pTempDst[iWrite] = pTempSrc[i];
		iWrite = (iWrite + 1) % m_pDataHead->iSize;
	}

	//首大于尾，直接写入（说明W-R之间可写，且一定不会跨越分段，一旦跨越分段iRead必然小于iWrite）
	if (iRead > iWrite)
	{
		memcpy((void *)&pbyCodeBuf[iWrite], (const void *)pIn, (size_t)nInLen);
	}
	else
	{
		//首小于尾,本包长大于右边剩余空间,需要分两段循环放到buff存放
		if ((int)nInLen > (m_pDataHead->iSize - iWrite))
		{
			//右边剩余buff
			int iRightLeftSize = m_pDataHead->iSize - iWrite;
			memcpy((void *)&pbyCodeBuf[iWrite], (const void *)&pIn[0], (size_t)iRightLeftSize);
			memcpy((void *)&pbyCodeBuf[0], (const void *)&pIn[iRightLeftSize], (size_t)(nInLen - iRightLeftSize));
		}
		//右边剩余buff够了，直接写入即可
		else
		{
			memcpy((void *)&pbyCodeBuf[iWrite], (const void *)&pIn[0], (size_t)nInLen);
		}
	}

	//更新尾偏移
	iWrite = (iWrite + nInLen) % m_pDataHead->iSize;
	m_pDataHead->iWrite = iWrite;

	return iWrite;
}
