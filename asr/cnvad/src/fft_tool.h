#ifndef __FFT_TOOL_H__
#define __FFT_TOOL_H__

// #define USE_POCKETFFT
#include "project_config.h"
#ifdef USE_POCKETFFT
#include "pocketfft.h"
#else
#include "fft.h"
#endif
#include "stdint.h"

typedef struct {
#ifdef USE_POCKETFFT
    rfftp_plan plan;
#endif
    float  win_buf[FRAMELEN];
} fft_ctx_t;

void fft_init(fft_ctx_t *fft_ctx);

void fft_forward(fft_ctx_t *fft_ctx, const int* input, int* output, float *fft_tmp);

int64_t get_amp_energy(int *amp);

void fft_uninit(fft_ctx_t *fft_ctx);

#endif
