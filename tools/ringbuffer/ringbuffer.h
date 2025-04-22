#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BUFF_MAX_LEN 16
#define VOS_OK 0
#define VOS_ERR -1

// char *pHead = NULL;         // 环形缓冲区首地址
// char *pValidRead = NULL;    // 已使用环形缓冲区首地址
// char *pValidWrite = NULL;   // 已使用环形缓冲区尾地址
// char *pTail = NULL;         // 环形缓冲区尾地址

typedef struct ring_buffer {
    char *pHead;         // head
    char *pValidRead;    // already used head
    char *pValidWrite;   // already used tail 
    char *pTail;         // tail
} ring_buffer;

void InitRingBuff(ring_buffer *r_buf); // Init ring buffer
void FreeRingBuff(ring_buffer *r_buf); // Release ring buffer

int WriteRingBuff(ring_buffer *r_buf, char *pBuff, int AddLen); // Write data to ring buffer
int ReadRingBuff(ring_buffer *r_buf, char *pBuff, int len); // Read data from ring buffer
