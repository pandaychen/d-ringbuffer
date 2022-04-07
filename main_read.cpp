#include "shm.h"
#include "ringqueue.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    ShmRingbuffer objAShm;
    objAShm.InitShm(SHM_SIZE_A, SHM_KEY_A);
    ShmRingQueue objAShmBuffQueue(objAShm.GetShmBuff());

    int nPackLen = 1024; //会被修正
    char buffPackData[1024 + 1] = {0};
    while (true)
    {
        objAShmBuffQueue.GetDataUnit(buffPackData, &nPackLen);
        printf("Read shm data len:%d\t data:%s\n", nPackLen, buffPackData);
        objAShmBuffQueue.PrintHead();
        usleep(1000000); // try about others
    }

    return 0;
}
