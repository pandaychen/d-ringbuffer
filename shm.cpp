#include "shm.h"
#include "log.h"
#include "node.h"
#include <errno.h>
#include <string.h>

//
ShmRingbuffer::ShmRingbuffer()
{
	m_iShmID = -1;
	m_iShmKey = -1;
	m_iShmSize = 0;
	m_pShmBuff = NULL;
}

ShmRingbuffer::ShmRingbuffer(size_t iShmSize, key_t iKey)
{
	InitShm(iShmSize, iKey);
}

// quit
ShmRingbuffer::~ShmRingbuffer()
{
	Detach();
}

char *ShmRingbuffer::GetShmBuff()
{
	return this->m_pShmBuff;
}

//	初始化共享内存
int ShmRingbuffer::InitShm(size_t iShmSize, key_t iKey)
{
	char szLog[4096] = {0};
	int iRet = SHM_ERROR;
	m_iShmID = shmget(iKey, iShmSize, IPC_CREAT | IPC_EXCL | 0666);
	if (m_iShmID < 0)
	{
		if (errno != EEXIST)
		{
			snprintf(szLog, 4095, "shmget failed, %s", strerror(errno));
			LOG_ERROR(szLog, ERRLOG_FILENAME);
			return SHM_ERROR;
		}
		//可能shm已经存在,尝试 Attach
		if ((m_iShmID = shmget(iKey, iShmSize, 0666)) < 0)
		{
			snprintf(szLog, 4095, "Attach to share memory %d failed, %s.Try to touch it", iKey, strerror(errno));
			LOG_INFO(szLog, INFOLOG_FILENAME);
			m_iShmID = shmget(iKey, 0, 0666);
			if (m_iShmID < 0)
			{
				memset(szLog, 0, 4096);
				snprintf(szLog, 4095, "touch shm failed: %s", strerror(errno));
				LOG_ERROR(szLog, ERRLOG_FILENAME);
				return SHM_ERROR;
			}
			else
			{
				memset(szLog, 0, 4096);
				snprintf(szLog, 4095, "Try remove the exist share memory %d", m_iShmID);
				LOG_INFO(szLog, INFOLOG_FILENAME);
				if (shmctl(m_iShmID, IPC_RMID, NULL))
				{
					memset(szLog, 0, 4096);
					snprintf(szLog, 4095, "Remove share memory failed, %s", strerror(errno));
					LOG_ERROR(szLog, ERRLOG_FILENAME);
					return SHM_ERROR;
					;
				}

				//再次尝试重新创建
				m_iShmID = shmget(iKey, iShmSize, IPC_CREAT | IPC_EXCL | 0666);
				if (m_iShmID < 0)
				{
					memset(szLog, 0, 4096);
					snprintf(szLog, 4095, "ReCreate memory failed, %s\n", strerror(errno));
					LOG_ERROR(szLog, ERRLOG_FILENAME);
					return SHM_ERROR;
				}
				iRet = CREATE_SUCC;
			}
		}
		else
		{
			memset(szLog, 0, 4096);
			snprintf(szLog, 4095, "attach to share memory %d succ", iKey);
			LOG_INFO(szLog, INFOLOG_FILENAME);
			iRet = ATTACH_SUCC;
		}
	}
	else
	{
		memset(szLog, 0, 4096);
		snprintf(szLog, 4095, "create share memory block, key = %08X, id = %d, size = %d \n", iKey, m_iShmID, iShmSize);
		LOG_INFO(szLog, INFOLOG_FILENAME);
		iRet = CREATE_SUCC;
	}

	//访问
	if ((m_pShmBuff = (char *)shmat(m_iShmID, NULL, 0)) == NULL)
	{
		memset(szLog, 0, 4096);
		snprintf(szLog, 4095, "access share memory %d failed", m_iShmID);
		LOG_ERROR(szLog, ERRLOG_FILENAME);
		return SHM_ERROR;
	}

	m_iShmSize = iShmSize;
	m_iShmKey = iKey;

	//初始化共享内存头
	NodeDataHead stDataHead;
	if (iRet == CREATE_SUCC)
	{
		stDataHead.iWrite = 0;
		stDataHead.iRead = 0;
		stDataHead.iOffset = sizeof(NodeDataHead);
		stDataHead.iSize = iShmSize - sizeof(NodeDataHead); //去掉head管理节点
		memcpy(m_pShmBuff, (char *)&stDataHead, sizeof(NodeDataHead));
	}
	else
	{
		memcpy((char *)&stDataHead, m_pShmBuff, sizeof(NodeDataHead));
		memset(szLog, 0, 4096);
		snprintf(szLog, 4095, "DataHead info: iSize:%u iBegin:%u iEnd:%u iOffset:%u", stDataHead.iSize, stDataHead.iWrite, stDataHead.iRead, stDataHead.iOffset);
		LOG_INFO(szLog, INFOLOG_FILENAME);
	}
	return iRet;
}

// 当前进程脱离共享内存，不删除
int ShmRingbuffer::Detach()
{
	int iRet = 0;
	if (m_pShmBuff != NULL)
	{
		iRet = shmdt(m_pShmBuff);
		m_pShmBuff = NULL;
	}

	return iRet;
}

int ShmRingbuffer::Delete()
{
	int iRet = 0;
	if (m_pShmBuff != NULL)
	{
		iRet = shmctl(m_iShmID, IPC_RMID, 0);
		m_pShmBuff = NULL;
	}

	return iRet;
}
