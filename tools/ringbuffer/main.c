#include "ringbuffer.h"

int main()
{
    char c;
    int len;
    int readLen;
    char readBuffer[10];
    int i = 0;

    ring_buffer *r_buf = malloc(sizeof(ring_buffer));
   
    InitRingBuff(r_buf);
   
    printf("Please enter a character\n");
   
    while(1) {
        c = getchar();
        switch(c) {
            case 'Q':
                goto exit;
            break;
            case 'R':
                readLen = ReadRingBuff(r_buf, readBuffer, 10);
                printf("ReadRingBufflen: %d\n", readLen);
                if (readLen > 0) {
                    for (i = 0; i < readLen; i++) {
                        printf("%c ",(char)readBuffer[i]);
                    }
                    printf("\n");
                }
            break;
            default :
                if (c != '\n') {
                    WriteRingBuff(r_buf, (char*)&c, 1);
                }
            break;
        }
    };
   
exit:
    FreeRingBuff(r_buf);

    return 0;
}