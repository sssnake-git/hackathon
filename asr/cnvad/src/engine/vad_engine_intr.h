#ifndef __VADIENG_INTR__
#define __VADIENG_INTR__

#include "ringarray.h"
//#define LOG_ENABLE
#define DBFIXED

#ifdef DBFIXED
#define DBTYPE int
#define ACCTYPE long long
#define FIXSHIFT 15
#define DB1 (1 << FIXSHIFT)
#define DB2 (2 << FIXSHIFT)
#define F1 ((float)DB1)
#define DBMAX ((int)(90.31 * DB1 + 0.5))
#define AVGINIT ((int)(40 * DB1))
#else
#define DBTYPE float
#define ACCTYPE float
#define FIXSHIFT 0
#define DBMAX 90.31
#define AVGINIT 40
#define DB1 1
#define DB2 2
#define F1 DB1
#endif

#define VADST_INNO 0x1
#define VADST_INVO 0x2
#define VADST_VOCF 0x4 // voice confirming
#define VADBF_FCUP 0x8 // vad avg buf force update

#define MINVOENG 30
#define MINVADTH 40
#define BASEFRMLENMS 8
#define NOISEAVGDURLONG 1000*10/8 //1000*10ms = 10s
#define NOISEAVGDURSHORT 100*10/8 //100*10ms = 1s
#define VOICEAVGDUR 100*10/8 //100*10ms = 1s
#define AVGBUFSIZE 100*10/8 //100*10ms = 1s
#define MINNPPEROID 56*10/8 //50*10ms = 500ms
#define AVGDURNORM 20*10/8 //20*10ms = 200ms
#define AVGDURSHORT 13 //10*10ms = 100ms 13*8=104
#define MINNP 1/10 // 1/10 MINNPPEROID

typedef struct {
    RingArray buf;
    int flag;
    int size;
} BufInfo;

typedef struct {
    RingArray* buf;
    int flag;
    int dur;
    ACCTYPE acc;
    DBTYPE avg;
} AvgDBCalcor;

void BufInfo_Init(BufInfo* bi, int size, int flag) {
    RingArray_Init(&bi->buf, size);
    bi->size = size;
    bi->flag = flag;
}

void BufInfo_Destroy(BufInfo* bi) {
    RingArray_Destroy(&bi->buf);
}

void AvgDBCalcor_Init(AvgDBCalcor* ac, BufInfo* bi, int dur) {
    ac->buf = &bi->buf;
    ac->flag = bi->flag;
    ac->dur = dur;
    ac->acc = 0;
    ac->avg = 0;
}

typedef struct {
    int nMsMinSpeech;                   /* minimum voice duration in ms before VAD actually detected the begining of the voice */
    int nMsMinTS;                       /* Minimum trailing silence duration in ms before VAD actually detected the end of the voice */
    float mGapCoef;                     /* gap coefficient, the larger the difficult to bos*/
    int nMsLeftLS;                      /* remained leading silence in ms. */
    int nMsLeftTS;                      /* remained trailing silence in ms. */

    int nMsShiftSize;                   /* window shift step in ms*/

    int nMinVocDb;                      /* min voice energe of db*/
    int nMinVadTh;                      /* min energe threshold to voice*/
    int nMsDetSpeech;                   /* pre-judged detected voice in ms */
    int nMsDetSpeech2;                  /* pre-judged detected voice in ms */
    int nMsSpConfirm;                   /* voice conferm time in ms */
    int nSpEngLowFrm;                   /* voice engerge lower then th in ms */
    int nMsDetTS;                       /* pre-judged deteded trailing silence */
    int nMsDetTS2;                      /* pre-judged deteded trailing silence */
    int nMsDetTS2Extra;                 /* pre-judged deteded trailing silence */
    int nMsEosDet;                      /* voice eos det time in ms */

    int nRealDetSpeech;                 /* real detected speech in ms */
    unsigned int nCurFrameIdx;          /* frame index counted from 1, not 0 */

    int curStatus;                      /* current vad status [invoice | innoise] */

    BufInfo dbBufs[3];
    AvgDBCalcor avgDbs[5];
    DBTYPE minNpBuf[MINNPPEROID];
    RingArray stAvgBuf;        /* continuous stAvg buf*/
    RingArray minStAvgBuf;     /* continuous stAvg buf*/
    RingArray upDownBuf;
    int minNpBufSize;
    int minNpPeriod;
    int engDecFrame;
    int engIncFrame;
    int detectEnding;
    DBTYPE stAvgMin;
    DBTYPE lst100Max;
#ifdef LOG_ENABLE
    FILE* logFp;
#endif
} Vad_Engine;

#endif
