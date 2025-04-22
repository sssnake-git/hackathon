#include <string.h>
#include <math.h>
#include "fft_tool.h"
#include <stdio.h>

#define PI 3.14159265

//hanning
static void gen_fft_win(int N, float *w) {
    int n;
    for (n = 0; n < N; n++) {
        *(w + n) = 0.5 - 0.5 * cos (2 * PI * n / N);
    }
}

void fft_init(fft_ctx_t *fft_ctx) {
#ifdef USE_POCKETFFT
    fft_ctx->plan = make_rfftp_plan(FFTLEN);
#endif
    gen_fft_win(FRAMELEN, fft_ctx->win_buf);
}

void fft_forward(fft_ctx_t *fft_ctx, const int* input, int* output, float *fft_buf) {
    float *win_buf = fft_ctx->win_buf;

    int i;
#if (FRAMELEN == FFTLEN)
    for (int i=0; i<FFTLEN; i++) {
        fft_buf[i] = input[i] * win_buf[i];
    }
#else
    for (i=0; i<(FFTLEN-FRAMELEN)/2; i++) {
        fft_buf[i] = 0;
    }
    float *fft_inptr = fft_buf + (FFTLEN-FRAMELEN)/2;
    for (i=0; i<FRAMELEN; i++) {
        fft_inptr[i] = input[i] * win_buf[i];
    }
    fft_inptr += FRAMELEN;
    for (i=0; i<(FFTLEN-FRAMELEN)/2; i++) {
        fft_inptr[i] = 0;
    }
#endif

#ifdef USE_POCKETFFT
    rfftp_forward(fft_ctx->plan, fft_buf, 1.0f);
#else
    ccrec_feat_rfft(fft_buf, FFTLEN, FFTLEN);
#endif
    // printf("\n");
    // for (i=0; i<FFTLEN; i++) {
    //     printf("%d ", (int)(fft_buf[i]));
    //     if (i%16 == 15)
    //         printf("\n");
    // }
    // printf("\n");

    if (fft_buf[0]>0) {
        output[0] = fft_buf[0]/FFTLEN;
    } else {
        output[0] = -fft_buf[0]/FFTLEN;
    }

#ifdef USE_POCKETFFT
    i=1;
    for (; i<FFTLEN/2; i++) {
        float amp = (float)sqrt(fft_buf[i*2-1]*fft_buf[i*2-1]+fft_buf[i*2]*fft_buf[i*2]);
        output[i] = amp/FFTLEN + 1;
    }
    if (fft_buf[i*2-1] > 0)
        output[i] = fft_buf[i*2-1]/FFTLEN + 1;
    else
        output[i] = -fft_buf[i*2-1]/FFTLEN + 1;
#else
    i=1;
    for (; i<FFTLEN/2; i++) {
        float amp = (float)sqrt(fft_buf[i*2]*fft_buf[i*2]+fft_buf[i*2+1]*fft_buf[i*2+1]);
        output[i] = amp/FFTLEN + 1;
    }
    if (fft_buf[1] > 0)
        output[i] = fft_buf[1]/FFTLEN + 1;
    else
        output[i] = -fft_buf[1]/FFTLEN + 1;
#endif
}

int64_t get_amp_energy(int *amp) {
    int i;
    int64_t engs = 0;
    for (i=0; i<FFTLEN/2; i+=4) {
        engs += amp[i]*amp[i];
        engs += amp[i+1]*amp[i+1];
        engs += amp[i+2]*amp[i+2];
        engs += amp[i+3]*amp[i+3];
    }
    engs += amp[i]*amp[i];
    return engs;
}

void fft_uninit(fft_ctx_t *fft_ctx) {
#ifdef USE_POCKETFFT
    destroy_rfftp_plan(fft_ctx->plan);
#endif
}
