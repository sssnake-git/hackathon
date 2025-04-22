#include "ringbuffer.h"

void InitRingBuff(ring_buffer *r_buf)
{
    r_buf->pHead = NULL;
    r_buf->pValidRead = NULL;
    r_buf->pValidWrite = NULL;
    r_buf->pTail = NULL;

    if (NULL == r_buf->pHead) {
        r_buf->pHead = (char *)malloc(BUFF_MAX_LEN * sizeof(char));
    }
   
    memset(r_buf->pHead, 0 , sizeof(BUFF_MAX_LEN));
   
    r_buf->pValidRead = r_buf->pHead;
    r_buf->pValidWrite = r_buf->pHead;
    r_buf->pTail = r_buf->pHead + BUFF_MAX_LEN;
}

void FreeRingBuff(ring_buffer *r_buf)
{
    if (NULL != r_buf->pHead) {
        free(r_buf->pHead);
    }
    free(r_buf);
}

int WriteRingBuff(ring_buffer *r_buf, char *pBuff, int AddLen)
{
    if (NULL == r_buf->pHead) {
        printf("WriteRingBuff:RingBuff is not Init!\n");
        return VOS_ERR;
    }

    if (AddLen > r_buf->pTail - r_buf->pHead) {
        printf("WriteRingBuff:New add buff is too long\n");
        return VOS_ERR;
    }
   
    // 若新增的数据长度大于写指针和尾指针之间的长度
    if (r_buf->pValidWrite + AddLen > r_buf->pTail) {
        int PreLen = r_buf->pTail - r_buf->pValidWrite;
        int LastLen = AddLen - PreLen;
        memcpy(r_buf->pValidWrite, pBuff, PreLen);
        memcpy(r_buf->pHead, pBuff + PreLen, LastLen);
       
        r_buf->pValidWrite = r_buf->pHead + LastLen;  //新环形缓冲区尾地址
    } else {
        memcpy(r_buf->pValidWrite, pBuff, AddLen); //将新数据内容添加到缓冲区
        r_buf->pValidWrite += AddLen;  //新的有效数据尾地址
    }
    return VOS_OK;
}

int ReadRingBuff(ring_buffer *r_buf, char *pBuff, int len)
{
    if (NULL == r_buf->pHead) {
        printf("ReadRingBuff:RingBuff is not Init!\n");
        return VOS_ERR;
    }

    if (len > r_buf->pTail - r_buf->pHead) {
        printf("ReadRingBuff:Read buff size is too long\n");   
        return VOS_ERR;
    }

    if (0 == len) {
        return VOS_OK;
    }

    if (r_buf->pValidRead + len > r_buf->pTail) {
        int PreLen = r_buf->pTail - r_buf->pValidRead;
        int LastLen = len - PreLen;
        memcpy(pBuff, r_buf->pValidRead, PreLen);
        memcpy(pBuff + PreLen, r_buf->pHead, LastLen);
       
        r_buf->pValidRead = r_buf->pHead + LastLen;
    } else {
        memcpy(pBuff, r_buf->pValidRead, len);
        r_buf->pValidRead += len;
    }
   
    return len;
}
