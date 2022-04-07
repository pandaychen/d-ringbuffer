#ifndef __SHM_H_
#define __SHM_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>

class ShmRingbuffer
{
public:
    ShmRingbuffer();
    ShmRingbuffer(size_t iShmSize, key_t iKey);
    ~ShmRingbuffer();

    //初始化
    int InitShm(size_t iShmSize, key_t iKey);

    char *GetShmBuff();

    int Detach();

    int Delete();

private:
    size_t m_iShmSize;
    key_t m_iShmKey;
    int m_iShmID;

    char *m_pShmBuff; //指向内存首地址
};

#endif
