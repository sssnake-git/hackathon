#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ccnn_core.h"
#include "fft_tool.h"
#include "ring_buf.h"
#include "project_config.h"
#include MDL_HEADER_H
#include "vad_engine.h"

#define max(a, b)    (((a) > (b)) ? (a) : (b))
#define min(a, b)    (((a) < (b)) ? (a) : (b))
#define SAMPLERATE 16000
#define FIX_MAGNIFY 4.0
#define DEBUG 0

typedef struct CNVAD_st {
    fft_ctx_t fft_ctx;
    void *vadinst;
    ringbuf_t ringbuf;
    short* tmp_cache;
    int nIdx;
    int outdelay;
    int sendextra;
    char sp_status;
    int wavbuf[FRAMELEN];
    int tmpbuf[FRAMELEN];
    int ampbuf[FFTLEN / 2 + 1];
    int opsize;
    int statesize;
    void* optable;
    uint8_t* mdlweights;
    int *mdlinput;
    int *mdloutput;
    int *mdlstate;
    int *mdltmp;
    uint8_t *rtdata;
} CNVAD_st;

void *vad_initialize(void *asr_handle) {
    CNVAD_st *_p_vad = (CNVAD_st *)malloc(sizeof(CNVAD_st));

    if (!_p_vad) {
        return NULL;
    }
    memset(_p_vad, 0, sizeof(CNVAD_st));

    int minSpeech = 100;
    int minTS = 200;
    float vadCoef = 1.0;
    int minVadTh = 35;
    int leadsil = 200;
    int trailsil = 200;

    _p_vad->ringbuf = ringbuf_new((max(minSpeech + 100, minTS) + 200) * 16 / FRAMESHIFT * FRAMESHIFT * sizeof(short));

    _p_vad->tmp_cache = malloc(FRAMESHIFT * sizeof(short));
    if (!_p_vad->ringbuf || !_p_vad->tmp_cache) {
        if (_p_vad->ringbuf)
            ringbuf_release(&_p_vad->ringbuf);
        if (_p_vad->tmp_cache)
            free(_p_vad->tmp_cache);
        free(_p_vad);
        return NULL;
    }

    vad_engine_init(&_p_vad->vadinst);
    // vad_engine_set_loglevel(_p_vad->vadinst, "2");

    vad_engine_set_param(_p_vad->vadinst, minSpeech, minTS, minVadTh, vadCoef, leadsil, trailsil);

    fft_init(&_p_vad->fft_ctx);

    int *iptr = (int*)ccnn_mdl;
    _p_vad->opsize = iptr[0];
    int rtbufsize = (iptr[2] + iptr[3]) * 4;
    _p_vad->rtdata = (uint8_t *)malloc(rtbufsize);
    _p_vad->statesize = iptr[2];
    _p_vad->optable = (uint8_t *)ccnn_mdl + sizeof(int) * 4;
    _p_vad->mdlweights = (uint8_t *)_p_vad->optable + ((_p_vad->opsize+3)&~3);
    _p_vad->mdlstate = (int*)(_p_vad->rtdata);
    _p_vad->mdltmp = _p_vad->mdlstate + iptr[2];
    if (_p_vad->mdlweights + iptr[1] - ccnn_mdl != sizeof(ccnn_mdl)) {
        fprintf(stderr, "ccnn formed mdl error size %d should be %d\n",
            (int)sizeof(ccnn_mdl), (int)sizeof(int) * 4 + ((_p_vad->opsize + 3)&~3) + iptr[1]);
        exit(-1);
    }

    _p_vad->mdlinput = _p_vad->ampbuf;
    _p_vad->mdloutput = _p_vad->tmpbuf;

    return _p_vad;
}

short vad_add_wav_data(void *_p_vad, short *in_data, short in_nSamples, short **out_data, unsigned int *bos,
		  unsigned int *eos, short *isBOS_or_EOS_Buffer) {
    if (!_p_vad) {
        out_data = NULL;
        return -1;
    }

    if (in_nSamples != FRAMESHIFT) {
        out_data = NULL;
        return -2;
    }

    *isBOS_or_EOS_Buffer = -1;
    CNVAD_st* cnvad = (CNVAD_st*)_p_vad;
    ringbuf_t ringbuf = cnvad->ringbuf;
    if (in_data) {
        int* input = cnvad->wavbuf;
        float* mdlout = (float*)cnvad->tmpbuf;
        int* readdst = input + FRAMELEN - FRAMESHIFT;

        int i;
        for (i=0; i<FRAMESHIFT; i++) {
            readdst[i] = in_data[i];
        }

        fft_forward(&cnvad->fft_ctx, input, cnvad->ampbuf, mdlout); //mdlout used as tmpbuf here

        int ret = ccnn_execute((void**)&cnvad->mdlweights, cnvad->optable, cnvad->opsize);

        int64_t engs = get_amp_energy(cnvad->ampbuf);
#ifdef FIX_MAGNIFY
        int outeng = engs*mdlout[0] * FIX_MAGNIFY;
#else
        int outeng = engs*mdlout[0];
#endif

        int vadoffset;
        int vadret = vad_engine_process(cnvad->vadinst, outeng, &vadoffset);

        memmove(input, input + FRAMESHIFT, (FRAMELEN-FRAMESHIFT)*sizeof(int));

        ringbuf_memcpy_into(ringbuf, in_data, in_nSamples*sizeof(short));

        if (vadret) {
            int offsetfrm = vadoffset*SAMPLERATE/(FRAMESHIFT * 1000);
            int offset_datalen = (vadoffset * SAMPLERATE / (FRAMESHIFT * 1000)) * (FRAMESHIFT * sizeof(short));
            int cached_len = ringbuf_bytes_used(ringbuf);
            if (cached_len > offset_datalen) {
                ringbuf_memcpy_from(NULL, ringbuf, cached_len - offset_datalen);
                cached_len = offset_datalen;
            }
            if (cached_len < offset_datalen) {
                offset_datalen = cached_len;
            }

            if (vadret == 1) {
                cnvad->outdelay = offset_datalen;
                cnvad->sp_status = 1;
                *isBOS_or_EOS_Buffer = 0;
                *bos = cnvad->nIdx - offsetfrm > 0 ? cnvad->nIdx - offsetfrm : 1;
            } else if (vadret == 2) {
                cnvad->sendextra = cnvad->outdelay - offset_datalen;
                if (cnvad->sendextra < 0) {
                    cnvad->sendextra = 0;
                }
                cnvad->sp_status = 2;
                *eos = cnvad->nIdx - offsetfrm > 0 ? cnvad->nIdx - offsetfrm : 1;
                // printf("sendextra %d\n", cnvad->sendextra);
            }
        }

        cnvad->nIdx ++;
    }

    int outsize = 0;
    if (cnvad->sp_status > 0) {
        void *ret = ringbuf_memcpy_from(cnvad->tmp_cache, ringbuf, FRAMESHIFT*sizeof(short));
        if (ret) {
            *out_data = cnvad->tmp_cache;
            outsize = FRAMESHIFT;
        }
        if (cnvad->sp_status == 2) {
            if (cnvad->sendextra > 0) {
                cnvad->sendextra -= FRAMESHIFT;
            }
            if (cnvad->sendextra <= 0 || !outsize) {
                *isBOS_or_EOS_Buffer = 1;
                cnvad->sp_status = 0;
            }
        }
    }
    return outsize;
}

void reset_vad_all(void *_p_vad) {
    if (!_p_vad)
        return;
    CNVAD_st* cnvad = (CNVAD_st*)_p_vad;
    vad_engine_full_reset(cnvad->vadinst);
    memset(cnvad->mdlstate, 0, cnvad->statesize*4);
    cnvad->sp_status = 0;
    cnvad->nIdx = 0;
}

void vad_reset(void *_p_vad) {
    if (!_p_vad)
        return;
    CNVAD_st* cnvad = (CNVAD_st*)_p_vad;
    vad_engine_reset(cnvad->vadinst);
    cnvad->sp_status = 0;
}

void vad_release(void *_p_vad) {
    if (_p_vad) {
        CNVAD_st* cnvad = (CNVAD_st*)_p_vad;
        fft_uninit(&cnvad->fft_ctx);
        vad_engine_close(cnvad->vadinst);
        if (cnvad->ringbuf)
            ringbuf_release(&cnvad->ringbuf);
        if (cnvad->tmp_cache)
            free(cnvad->tmp_cache);
        if (cnvad->rtdata)
            free(cnvad->rtdata);
        free(cnvad);
    }
}
