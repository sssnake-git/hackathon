#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "dqvad_intr.h"
#include "dqvad.h"

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

static float* GapMagic  = NULL;
#define maigcGap(x, c) \
    ((DBTYPE)((DBMAX-x)*GapMagic[(int)(90.31-(x/DB1))]*c))

void DqVad_UpdateDbBufs(DqVad* vad, DBTYPE currDB);
void DqVad_EosReset(DqVad* vad);

void cnvad_engine_init(void** self) {
    DqVad* vad = (DqVad*)malloc(sizeof(DqVad));
    if (vad) {
        vad->nMsMinSpeech = 100;
        vad->nMsMinTS = 500;
        vad->mGapCoef = 1.0;
        vad->nMsLeftLS = 150;
        vad->nMsLeftTS = 200;
        vad->nMsShiftSize = 8;
        vad->nMsDetSpeech = 0;
        vad->nMsDetSpeech2 = 0;
        vad->nMsSpConfirm = 0;
        vad->nSpEngLowFrm = 0;
        vad->nMsDetTS = 0;
        vad->nMsDetTS2 = 0;
        vad->nMsDetTS2Extra = 0;
        vad->nMsEosDet = 0;
        vad->nRealDetSpeech = 0;
        vad->nCurFrameIdx = 0;
        BufInfo_Init(&vad->dbBufs[0], NOISEAVGDURLONG, VADST_INNO);
        BufInfo_Init(&vad->dbBufs[1], NOISEAVGDURSHORT, VADST_INNO|VADST_VOCF);
        BufInfo_Init(&vad->dbBufs[2], AVGBUFSIZE, VADST_INNO|VADST_VOCF|VADST_INVO);
        AvgDBCalcor_Init(&vad->avgDbs[0], &vad->dbBufs[0], NOISEAVGDURLONG);
        AvgDBCalcor_Init(&vad->avgDbs[1], &vad->dbBufs[0], NOISEAVGDURSHORT);
        AvgDBCalcor_Init(&vad->avgDbs[2], &vad->dbBufs[1], NOISEAVGDURSHORT);
        AvgDBCalcor_Init(&vad->avgDbs[3], &vad->dbBufs[2], AVGDURNORM);
        AvgDBCalcor_Init(&vad->avgDbs[4], &vad->dbBufs[2], AVGDURSHORT);

        int i;
        for (i=0; i<2; i++) {
            RingArray_Write(&vad->dbBufs[0].buf, AVGINIT);
            vad->avgDbs[i].acc = AVGINIT*2;
            vad->avgDbs[i].avg = AVGINIT;
        }
        RingArray_Write(&vad->dbBufs[2].buf, 0);
        vad->minNpBufSize = MINNPPEROID*MINNP;
        vad->minNpPeriod = MINNPPEROID;
        for (int i=0; i<vad->minNpBufSize; i++) {
            vad->minNpBuf[i] = DBMAX;
        }
        RingArray_Init(&vad->stAvgBuf, 12);
        RingArray_Init(&vad->minStAvgBuf, 12);
        RingArray_Init(&vad->upDownBuf, 12);
        vad->stAvgMin = DBMAX;
        vad->lst100Max = 0;
        vad->engDecFrame = 0;
        vad->engIncFrame = 0;
        vad->detectEnding = 0;
#ifdef LOG_ENABLE
        vad->logFp = 0;
#endif
#ifdef DBFIXED
        vad->nMinVocDb = MINVOENG << FIXSHIFT;
        vad->nMinVadTh = MINVADTH << FIXSHIFT;
#else
        vad->nMinVocDb = MINVOENG;
        vad->nMinVadTh = MINVADTH;
#endif
        vad->curStatus = VADST_INNO;
        if (!GapMagic) {
            GapMagic  = malloc(92*sizeof(float));
            for (i=0; i<92; i++) {
                GapMagic[i] = 0.2*pow(i+1, (float)1/90);
            }
        }
    }
    *self = vad;
}

void cnvad_engine_set_param(void* self, int MinSpeech, int MinTS, int MinVadTh, float gapCoef, int LS, int TS) {
    DqVad* vad = (DqVad*)self;
    vad->nMsMinSpeech = MinSpeech;
    vad->nMsMinTS = MinTS;
#ifdef DBFIXED
    vad->nMinVadTh = MinVadTh << FIXSHIFT;
#else
    vad->nMinVadTh = MinVadTh;
#endif
    vad->mGapCoef = gapCoef;
    vad->nMsLeftLS = LS;
    vad->nMsLeftTS = TS;
    
    if (vad->nMsLeftTS > vad->nMsMinTS) {
        printf("Remained TS should not exceed MinTS!\n");
        exit(1);
    }
}

int cnvad_engine_process(void* self, int eng, int *offset) {
    DqVad* vad = (DqVad*)self;

    int ret = 0;
    vad->nCurFrameIdx ++;
    int nMsShiftSize = vad->nMsShiftSize;
    int curStatus = vad->curStatus;
    AvgDBCalcor *avgDbs = vad->avgDbs;
    DBTYPE *minNpBuf = vad->minNpBuf;
    RingArray *stAvgBuf = &vad->stAvgBuf;
    RingArray *minStAvgBuf = &vad->minStAvgBuf;
    int minNpPeriod = vad->minNpPeriod;
    DBTYPE stAvgMin = vad->stAvgMin;
    DBTYPE lst100Max = vad->lst100Max;

    DBTYPE fCurrDB;
#ifdef DBFIXED
    fCurrDB = 4.343*log(eng+1) * (1 << FIXSHIFT);
#else
    fCurrDB = 4.343*log(eng+1);
#endif
    fCurrDB = max(fCurrDB, vad->nMinVocDb);
    DqVad_UpdateDbBufs(vad, fCurrDB);

    DBTYPE newmin = fCurrDB;
    RingArray *npBuf = &vad->dbBufs[2].buf;
    // update minNpBuf
    if (RingArray_Size(npBuf) > minNpPeriod) {
        DBTYPE hisOut = RingArray_Get(npBuf, -(minNpPeriod+1));
        DBTYPE minNpmax = minNpBuf[0];
        if (minNpmax >= hisOut) {
            int i=vad->minNpBufSize-1;
            for (; i>=0; i--) {
                if (minNpBuf[i] == hisOut)
                    break;
            }
            for (;i>0; i--) {
                minNpBuf[i] = minNpBuf[i-1];
            }
            minNpBuf[0] = DBMAX;
            minNpmax = minNpBuf[1];
            for (i=-minNpPeriod; i<0; i++) {
                int npi = RingArray_Get(npBuf, i);
                if (npi < newmin && npi > minNpmax)
                    newmin  = npi;
            }
        }
    }
    if (newmin < minNpBuf[0]) {
        int i=0;
        for (; i<vad->minNpBufSize-1; i++) {
            if (newmin > minNpBuf[i+1])
                break;
            minNpBuf[i] = minNpBuf[i+1];
        }
        minNpBuf[i] = newmin;
    }

    DBTYPE eosth = min(min(avgDbs[0].avg, avgDbs[1].avg), minNpBuf[0]+DB1);
    DBTYPE vadth = max(eosth, vad->nMinVadTh);
    DBTYPE normAvg = avgDbs[3].avg;
    DBTYPE stAvg = avgDbs[4].avg;
    if (RingArray_IsFull(stAvgBuf)) {
        DBTYPE drop = RingArray_Get(stAvgBuf, 0);
        RingArray_Drop(stAvgBuf, 1);
        stAvgMin = drop<=stAvgMin?RingArray_Min(stAvgBuf, DBMAX):stAvgMin;
    }
    if (RingArray_Size(npBuf) > 10) {
        DBTYPE out = RingArray_Get(npBuf, -11);
        lst100Max = out>=lst100Max?RingArray_Max(npBuf, 10):lst100Max;
    }
    lst100Max = fCurrDB>=lst100Max?fCurrDB:lst100Max;
    RingArray_Write(stAvgBuf, stAvg);
    stAvgMin = stAvg<=stAvgMin?stAvg:stAvgMin;
    if (RingArray_IsFull(minStAvgBuf)) {
        RingArray_Drop(minStAvgBuf, 1);
    }
    RingArray_Write(minStAvgBuf, stAvgMin);
    DBTYPE gap = maigcGap(vadth, vad->mGapCoef);

#ifdef LOG_ENABLE
    if (vad->logFp) {
        DBTYPE varError = fCurrDB - vadth;
        fprintf(vad->logFp, "%.2f: %d,%d-%d:%d,%d\t%.2fdb th %.2f Err %.2f gap %.2f \tmin: %.2f", vad->nCurFrameIdx*8/1000.0,curStatus,vad->nMsDetSpeech,vad->nMsDetTS,vad->nMsDetTS2,vad->nMsEosDet,fCurrDB/F1,vadth/F1,varError/F1,gap/F1,minNpBuf[0]/F1);
        fprintf(vad->logFp, "\tavg %.2f savg %.2f min %.2f max %.2f", normAvg/F1, stAvg/F1, RingArray_Get(minStAvgBuf, 0)/F1, lst100Max/F1);
        fprintf(vad->logFp, "\n");
    }
#endif

    if (curStatus & (VADST_INNO|VADST_VOCF)) {
        if (fCurrDB > vadth) {
            vad->nMsDetSpeech += nMsShiftSize;
            if (vad->nMsDetSpeech2 > 0 || fCurrDB > vadth + gap/4)
                vad->nMsDetSpeech2 += nMsShiftSize;
            vad->nSpEngLowFrm = 0;

            if (curStatus == VADST_INNO && fCurrDB > vadth + gap) {
                if (vad->nMsDetSpeech >= vad->nMsMinSpeech) {
                    curStatus = VADST_VOCF;
                }
            }
            if (curStatus == VADST_VOCF) {
                if (fCurrDB > vadth + gap/2)
                    vad->nMsSpConfirm += nMsShiftSize;
                if (stAvg > vadth + gap ||
                    fCurrDB > vadth + gap*2 ||
                    vad->nMsSpConfirm > vad->nMsMinSpeech) {
                    *offset = vad->nMsDetSpeech2 + vad->nMsLeftLS;
                    vad->nRealDetSpeech = vad->nMsMinSpeech;
                    vad->nMsDetSpeech = 0;
                    vad->nMsDetSpeech2 = 0;
                    vad->nMsSpConfirm = 0;
                    curStatus = VADST_INVO;
                    ret = 1;
                } else {
                    if (stAvg < avgDbs[2].avg) {
                        curStatus = VADST_INNO;
                        vad->nMsDetSpeech = 0;
                        vad->nMsDetSpeech2 = 0;
                    }
                }
            }
        } else {
            vad->nSpEngLowFrm ++;
            vad->nMsSpConfirm = 0;
            if (vad->nCurFrameIdx < 30 && vad->nMsDetSpeech2 > 0)
                vad->nMsDetSpeech2 += nMsShiftSize;
            if (stAvg < vadth || vad->nSpEngLowFrm > 8) {
                curStatus = VADST_INNO;
                vad->nMsDetSpeech = 0;
                vad->nMsDetSpeech2 = 0;
            }
        }
    } else {
        int nMsEosDet = vad->nMsEosDet;
        RingArray *upDownBuf = &vad->upDownBuf;
        int engDecFrame = vad->engDecFrame;
        int engIncFrame = vad->engIncFrame;
        int detectEnding = vad->detectEnding;

        RingArray *hbuf = &vad->dbBufs[2].buf;
        DBTYPE cur2 = (fCurrDB + RingArray_Get(hbuf, -2))/2;
        DBTYPE dither = max(cur2 - RingArray_Get(minStAvgBuf, 0), abs(lst100Max- cur2));
        DBTYPE dithergap = maigcGap(cur2, vad->mGapCoef);

        int udsum = RingArray_Sum(upDownBuf, 0);
        if (RingArray_IsFull(upDownBuf))
            RingArray_Drop(upDownBuf, 1);
        if (fCurrDB < min(normAvg, RingArray_Get(hbuf, -2))) { // energe go down
            RingArray_Write(upDownBuf, -1);
            if (detectEnding == 1) {
                if (engIncFrame > 2) {
                    detectEnding = 2;
                } else {
                    engDecFrame = 0;
                    detectEnding = 0;
                }
            }
            engDecFrame += 1;
            if (!detectEnding) {
                if (engDecFrame >= 5 && RingArray_Get(hbuf, -5) < vadth) {
                    if ((RingArray_Get(hbuf, -5) - fCurrDB)/4 < gap/4) {
                        for (int i=-2; i<=-5; i--) {
                            if (RingArray_Get(hbuf, i) > vadth)
                                break;
                            vad->nMsDetTS2 += nMsShiftSize;
                        }
                        // printf("detectEnding 2\n");
                        detectEnding = 2;
                    }
                }
            }
            engIncFrame = 0;
        } else {
            engIncFrame ++;
            RingArray_Write(upDownBuf, fCurrDB==RingArray_Get(hbuf, -2)?0:1);
            if (!detectEnding) {
                if ((engDecFrame >= 5 && (fCurrDB < vadth && udsum > -2 && udsum < 2)) || fCurrDB < eosth) {
                    detectEnding = 1;
                } else {
                    engDecFrame = 0;
                }
            }
            if (detectEnding == 1) {
                if (engIncFrame <= RingArray_Size(hbuf)) {
                    if (fCurrDB - RingArray_Get(hbuf, -(engIncFrame+1)) < engIncFrame*(gap/5)) {
                        detectEnding = 2;
                        engDecFrame = 0;
                    } else if (fCurrDB - RingArray_Get(hbuf, -(engIncFrame+1)) > gap/3 ||
                               fCurrDB > vadth + gap) {
                        detectEnding = 0;
                        engDecFrame = 0;
                    }
                } else {
                    detectEnding = 2;
                    engDecFrame = 0;
                }
            } else {
                engDecFrame = 0;
            }
        }

        if (min(fCurrDB, stAvg) < vadth) {
            if (vad->nMsDetTS !=0) {
                vad->nMsDetTS += nMsShiftSize;
                nMsEosDet += nMsShiftSize;
            } else {
                if (udsum > -3 && udsum < 3) {
                    nMsEosDet = vad->nMsDetTS = nMsShiftSize;
                } else if (stAvg < vadth + gap/8) {
                    nMsEosDet = vad->nMsDetTS = 5*nMsShiftSize;
                }
            }
            if (dither > dithergap/2) {
                nMsEosDet = 0;
            }
        } else {
            if ((fCurrDB + RingArray_Get(hbuf, -2))/2 > vadth + gap/8) {
                vad->nMsDetTS = 0;
                nMsEosDet = 0;
            }
        }

        if (detectEnding) {
            if (detectEnding == 2)
                vad->nMsDetTS2 += nMsShiftSize;
            if (fCurrDB > vadth + gap ||
                (udsum > 5 && dither > dithergap)) {
#ifdef LOG_ENABLE
                if (vad->logFp)
                    fprintf(vad->logFp, "reset dither %.2f %.2f %.2f %.2f\n", dither/F1, gap/3/F1, (fCurrDB - RingArray_Get(minStAvgBuf, 0))/F1, abs(fCurrDB - lst100Max)/F1);
#endif
                detectEnding = 0;
                engDecFrame = 0;
                vad->nMsDetTS2Extra = 0;
                vad->nMsDetTS2 = 0;
                nMsEosDet = 0;
            }
        } else {
            if (dither < dithergap/2) {
                detectEnding = 2;
                vad->nMsDetTS2Extra = nMsShiftSize*10;
            }
        }
        int nMsMinTS = vad->nMsMinTS;

        int udsum4 = RingArray_Sum(upDownBuf, 4);
        if (nMsEosDet >= nMsMinTS && (detectEnding == 2 || (-2 <= udsum4 && udsum4 <= 2))) {
            // printf("eos 1\n");
            *offset = vad->nMsDetTS - vad->nMsLeftTS;
            DqVad_EosReset(vad);
            ret = 2;
        } else if (vad->nMsDetTS2 >= nMsMinTS &&  (-2 <= udsum4 && udsum4 <= 2)) {
            // printf("eos 2\n");
            *offset = vad->nMsDetTS2 - (vad->nMsLeftTS + vad->nMsDetTS2Extra);
            DqVad_EosReset(vad);
            ret = 2;
        } else {
#ifdef LOG_ENABLE
            if (vad->logFp && (vad->nMsDetTS2 >= nMsMinTS || nMsEosDet >= nMsMinTS))
                fprintf(vad->logFp, "udsum4 %d\n",udsum4);
#endif
            if (vad->nMsDetTS > nMsMinTS/2 && fCurrDB > eosth) {
                if (RingArray_Get(hbuf, -(min(vad->nMsDetTS, NOISEAVGDURSHORT)/nMsShiftSize)) > fCurrDB + gap/4) {
                    if (vad->nMsDetTS > nMsShiftSize*2)
                        vad->nMsDetTS -= nMsShiftSize;
                }
            }
            if (vad->nMsDetTS2 > nMsMinTS/2 && fCurrDB > eosth) {
                if (RingArray_Get(hbuf, -(min(vad->nMsDetTS2, NOISEAVGDURSHORT)/nMsShiftSize)) > fCurrDB + gap/4)
                    if (vad->nMsDetTS2 > nMsShiftSize*2)
                        vad->nMsDetTS2 -= nMsShiftSize;
            }
            vad->nRealDetSpeech += nMsShiftSize;
        }
        if (ret == 2) {
            engDecFrame = 0;
            curStatus = VADST_INNO;
        } else {
            vad->nMsEosDet = nMsEosDet;
            vad->engDecFrame = engDecFrame;
            vad->engIncFrame = engIncFrame;
            vad->detectEnding = detectEnding;
        }
    }

    vad->curStatus = curStatus;
    vad->stAvgMin = stAvgMin;
    vad->lst100Max = lst100Max;

    return ret;
}

void DqVad_UpdateDbBufs(DqVad* vad, DBTYPE currDB) {
    for (int i = 0; i < 5; i++) {
        AvgDBCalcor* avgDb = &vad->avgDbs[i];
        if (avgDb->flag & vad->curStatus) {
            if (RingArray_Size(avgDb->buf) >= avgDb->dur)
                avgDb->acc -= RingArray_Get(avgDb->buf, -avgDb->dur);
        }
    }
    for (int i = 0; i < 3; i++) {
        BufInfo *dbBuf = &vad->dbBufs[i];
        if (dbBuf->flag & vad->curStatus) {
            if (RingArray_IsFull(&dbBuf->buf))
                RingArray_Drop(&dbBuf->buf, 1);
            RingArray_Write(&dbBuf->buf, currDB);
        }
    }
    for (int i = 0; i < 5; i++) {
        AvgDBCalcor* avgDb = &vad->avgDbs[i];
        if (avgDb->flag & vad->curStatus) {
            avgDb->acc += currDB;
            avgDb->avg = avgDb->acc / min(RingArray_Size(avgDb->buf), avgDb->dur);
        }
    }
}

void DqVad_EosReset(DqVad* vad) {
    vad->nMsDetSpeech = 0;
    vad->nMsDetSpeech2 = 0;
    vad->nMsDetTS = 0;
    vad->nMsDetTS2 = 0;
    vad->nMsDetTS2Extra = 0;
    vad->nRealDetSpeech = 0;
    vad->nMsEosDet = 0;
    vad->engDecFrame = 0;
    vad->engIncFrame = 0;
    vad->detectEnding = 0;
    vad->curStatus = VADST_INNO;
}

void DqVad_FullReset(void* self) {
    DqVad* vad = (DqVad*)self;
    int i;
    for (i=0; i<2; i++) {
        RingArray_Clear(&vad->dbBufs[i].buf);
        RingArray_Write(&vad->dbBufs[0].buf, AVGINIT);
        vad->avgDbs[i].acc = AVGINIT*2;
        vad->avgDbs[i].avg = AVGINIT;
    }
    for (; i<5; i++) {
        vad->avgDbs[i].acc = 0;
        vad->avgDbs[i].avg = 0;
    }
    RingArray_Clear(&vad->dbBufs[2].buf);
    RingArray_Write(&vad->dbBufs[2].buf, 0);
    for (i=0; i<vad->minNpBufSize; i++) {
        vad->minNpBuf[i] = DBMAX;
    }
    RingArray_Clear(&vad->stAvgBuf);
    RingArray_Clear(&vad->minStAvgBuf);
    RingArray_Clear(&vad->upDownBuf);
    vad->stAvgMin = DBMAX;
    vad->lst100Max = 0;
    vad->nCurFrameIdx = 0;
    DqVad_EosReset(vad);
}

void DqVad_Reset(void* self) {
    DqVad* vad = (DqVad*)self;
    DqVad_EosReset(vad);
}

#ifdef LOG_ENABLE
void DqVad_SetLogOut(void* self, char* path) {
    DqVad* vad = (DqVad*)self;
    if (!path) {
        if (vad->logFp && vad->logFp != stdout && vad->logFp != stderr) {
            fclose(vad->logFp);
        }
        vad->logFp = 0;
    } else if (strcmp(path, "1") == 0) {
        vad->logFp = stdout;
    } else if (strcmp(path, "2") == 0) {
        vad->logFp = stderr;
    } else {
        vad->logFp = fopen(path, "w");
    }
}
#endif

void DqVad_Close(void* self) {
    if (self) {
        DqVad* vad = (DqVad*)self;
        BufInfo_Destroy(&vad->dbBufs[0]);
        BufInfo_Destroy(&vad->dbBufs[1]);
        BufInfo_Destroy(&vad->dbBufs[2]);
        RingArray_Destroy(&vad->stAvgBuf);
        RingArray_Destroy(&vad->minStAvgBuf);
        RingArray_Destroy(&vad->upDownBuf);
#ifdef LOG_ENABLE
        if (vad->logFp && vad->logFp != stdout && vad->logFp != stderr)
            fclose(vad->logFp);
#endif
        free(vad);
    }
}
