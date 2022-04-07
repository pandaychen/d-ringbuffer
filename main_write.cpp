#include "shm.h"
#include "ringqueue.h"
#include "node.h"
#include <stdlib.h>
#include <string.h>

const char *TEST_DATA = "hello world!";

int main(int argc, char **argv)
{
    ShmRingbuffer objAShm;
    objAShm.InitShm(SHM_SIZE_A, SHM_KEY_A);
    ShmRingQueue objAShmBuffQueue(objAShm.GetShmBuff());

    char buffPackData[1024] = {0};
    int nPackLen = strlen(TEST_DATA);
    memcpy(buffPackData, TEST_DATA, nPackLen);
    while (true)
    {
        objAShmBuffQueue.PutDataUnit(buffPackData, nPackLen);
        printf("Write To shm data len:%d\t data:%s\n", nPackLen, buffPackData);

        objAShmBuffQueue.PrintHead();
        usleep(1000000); // try about others
    }

    return 0;
}
