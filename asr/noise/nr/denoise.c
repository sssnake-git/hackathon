#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "denoise.h"
#include "smallft.h"

static float hamin[2 * NN] = {
    0.0800, 0.0800, 0.0801, 0.0803, 0.0806, 0.0809, 0.0813, 0.0817, 0.0822,
    0.0828, 0.0835, 0.0842, 0.0850, 0.0859, 0.0868, 0.0878, 0.0889, 0.0900,
    0.0912, 0.0925, 0.0938, 0.0953, 0.0967, 0.0983, 0.0999, 0.1016, 0.1033,
    0.1051, 0.1070, 0.1089, 0.1109, 0.1130, 0.1152, 0.1174, 0.1196, 0.1219,
    0.1243, 0.1268, 0.1293, 0.1319, 0.1345, 0.1372, 0.1400, 0.1428, 0.1457,
    0.1486, 0.1516, 0.1547, 0.1578, 0.1610, 0.1642, 0.1675, 0.1709, 0.1743,
    0.1777, 0.1812, 0.1848, 0.1884, 0.1921, 0.1958, 0.1996, 0.2034, 0.2073,
    0.2113, 0.2152, 0.2193, 0.2233, 0.2275, 0.2316, 0.2359, 0.2401, 0.2444,
    0.2488, 0.2532, 0.2576, 0.2621, 0.2667, 0.2712, 0.2758, 0.2805, 0.2852,
    0.2899, 0.2947, 0.2995, 0.3043, 0.3092, 0.3141, 0.3190, 0.3240, 0.3290,
    0.3341, 0.3391, 0.3442, 0.3494, 0.3545, 0.3597, 0.3649, 0.3702, 0.3755,
    0.3808, 0.3861, 0.3914, 0.3968, 0.4022, 0.4076, 0.4130, 0.4184, 0.4239,
    0.4294, 0.4349, 0.4404, 0.4459, 0.4515, 0.4570, 0.4626, 0.4682, 0.4738,
    0.4794, 0.4850, 0.4906, 0.4962, 0.5019, 0.5075, 0.5131, 0.5188, 0.5244,
    0.5301, 0.5358, 0.5414, 0.5471, 0.5527, 0.5584, 0.5640, 0.5697, 0.5753,
    0.5810, 0.5866, 0.5922, 0.5978, 0.6034, 0.6090, 0.6146, 0.6202, 0.6258,
    0.6313, 0.6368, 0.6424, 0.6479, 0.6534, 0.6588, 0.6643, 0.6697, 0.6751,
    0.6805, 0.6859, 0.6913, 0.6966, 0.7019, 0.7072, 0.7124, 0.7177, 0.7229,
    0.7280, 0.7332, 0.7383, 0.7434, 0.7485, 0.7535, 0.7585, 0.7634, 0.7684,
    0.7733, 0.7781, 0.7829, 0.7877, 0.7925, 0.7972, 0.8018, 0.8065, 0.8111,
    0.8156, 0.8201, 0.8246, 0.8290, 0.8334, 0.8377, 0.8420, 0.8463, 0.8505,
    0.8546, 0.8587, 0.8628, 0.8668, 0.8707, 0.8746, 0.8785, 0.8823, 0.8860,
    0.8897, 0.8934, 0.8970, 0.9005, 0.9040, 0.9074, 0.9108, 0.9141, 0.9174,
    0.9206, 0.9237, 0.9268, 0.9299, 0.9328, 0.9358, 0.9386, 0.9414, 0.9441,
    0.9468, 0.9494, 0.9520, 0.9544, 0.9569, 0.9592, 0.9615, 0.9638, 0.9659,
    0.9680, 0.9701, 0.9720, 0.9740, 0.9758, 0.9776, 0.9793, 0.9809, 0.9825,
    0.9840, 0.9855, 0.9868, 0.9882, 0.9894, 0.9906, 0.9917, 0.9927, 0.9937,
    0.9946, 0.9954, 0.9962, 0.9969, 0.9975, 0.9980, 0.9985, 0.9989, 0.9993,
    0.9996, 0.9998, 0.9999, 1.0000, 1.0000, 0.9999, 0.9998, 0.9996, 0.9993,
    0.9989, 0.9985, 0.9980, 0.9975, 0.9969, 0.9962, 0.9954, 0.9946, 0.9937,
    0.9927, 0.9917, 0.9906, 0.9894, 0.9882, 0.9868, 0.9855, 0.9840, 0.9825,
    0.9809, 0.9793, 0.9776, 0.9758, 0.9740, 0.9720, 0.9701, 0.9680, 0.9659,
    0.9638, 0.9615, 0.9592, 0.9569, 0.9544, 0.9520, 0.9494, 0.9468, 0.9441,
    0.9414, 0.9386, 0.9358, 0.9328, 0.9299, 0.9268, 0.9237, 0.9206, 0.9174,
    0.9141, 0.9108, 0.9074, 0.9040, 0.9005, 0.8970, 0.8934, 0.8897, 0.8860,
    0.8823, 0.8785, 0.8746, 0.8707, 0.8668, 0.8628, 0.8587, 0.8546, 0.8505,
    0.8463, 0.8420, 0.8377, 0.8334, 0.8290, 0.8246, 0.8201, 0.8156, 0.8111,
    0.8065, 0.8018, 0.7972, 0.7925, 0.7877, 0.7829, 0.7781, 0.7733, 0.7684,
    0.7634, 0.7585, 0.7535, 0.7485, 0.7434, 0.7383, 0.7332, 0.7280, 0.7229,
    0.7177, 0.7124, 0.7072, 0.7019, 0.6966, 0.6913, 0.6859, 0.6805, 0.6751,
    0.6697, 0.6643, 0.6588, 0.6534, 0.6479, 0.6424, 0.6368, 0.6313, 0.6258,
    0.6202, 0.6146, 0.6090, 0.6034, 0.5978, 0.5922, 0.5866, 0.5810, 0.5753,
    0.5697, 0.5640, 0.5584, 0.5527, 0.5471, 0.5414, 0.5358, 0.5301, 0.5244,
    0.5188, 0.5131, 0.5075, 0.5019, 0.4962, 0.4906, 0.4850, 0.4794, 0.4738,
    0.4682, 0.4626, 0.4570, 0.4515, 0.4459, 0.4404, 0.4349, 0.4294, 0.4239,
    0.4184, 0.4130, 0.4076, 0.4022, 0.3968, 0.3914, 0.3861, 0.3808, 0.3755,
    0.3702, 0.3649, 0.3597, 0.3545, 0.3494, 0.3442, 0.3391, 0.3341, 0.3290,
    0.3240, 0.3190, 0.3141, 0.3092, 0.3043, 0.2995, 0.2947, 0.2899, 0.2852,
    0.2805, 0.2758, 0.2712, 0.2667, 0.2621, 0.2576, 0.2532, 0.2488, 0.2444,
    0.2401, 0.2359, 0.2316, 0.2275, 0.2233, 0.2193, 0.2152, 0.2113, 0.2073,
    0.2034, 0.1996, 0.1958, 0.1921, 0.1884, 0.1848, 0.1812, 0.1777, 0.1743,
    0.1709, 0.1675, 0.1642, 0.1610, 0.1578, 0.1547, 0.1516, 0.1486, 0.1457,
    0.1428, 0.1400, 0.1372, 0.1345, 0.1319, 0.1293, 0.1268, 0.1243, 0.1219,
    0.1196, 0.1174, 0.1152, 0.1130, 0.1109, 0.1089, 0.1070, 0.1051, 0.1033,
    0.1016, 0.0999, 0.0983, 0.0967, 0.0953, 0.0938, 0.0925, 0.0912, 0.0900,
    0.0889, 0.0878, 0.0868, 0.0859, 0.0850, 0.0842, 0.0835, 0.0828, 0.0822,
    0.0817, 0.0813, 0.0809, 0.0806, 0.0803, 0.0801, 0.0800, 0.0800};

extern void *spx_fft_init(int size);
extern void spx_ifft(void *table, float *in, float *out);
extern void spx_fft(void *table, float *in, float *out);

void spec_sub_ini(specsub *sub) {
    int ii;

    memset(sub->sbuf, 0, NN * sizeof(short));
    // memset(sub->insign, 0, 2 * NN * sizeof(float));
    memset(sub->overlap, 0, NN * sizeof(float));
    sub->Thres = 3;
    sub->alpha = 2.0;

    sub->FLOOR = 0.1;
    sub->G = 0.9;
    sub->winGain = 0;
    sub->framecount = 0;
    sub->initial_noise_frame = 10;
    // sub->winGain=NN/sum(win);

    for (ii = 0; ii < 2 * NN; ii++) {
        // hamin[ii] = (float)(hamin_tmp[ii] * 0.0001);
        sub->winGain += hamin[ii];
    }
    sub->winGain = NN / sub->winGain;
    sub->fft_lookup = spx_fft_init(2 * NN);
}

void spec_sub_deini(specsub *sub) {
    // spx_fft_destroy(sub->fft_lookup);
    // free(sub);
}

float get_snr(specsub *sub) { return sub->snr; }

float berouti(float snr) {
    float beta = 0.0;
    if ((snr >= -5.0) && (snr <= 20)) {
        beta = 4 - snr * 3 / 20;
    } else {
        if (snr < -5.0) beta = 5;
        if (snr > 20) beta = 1;
    }

    return beta;
}

static float fftbuf[2 * NN];
// static float ps[NN];
static float sigpow[NN];
// static float ifftbuf[2 * NN];
void spec_sub(specsub *sub, short *in) {
    int ii;
    int winGain = 0;
    /* 
    float ifftbuf[2 * NN];
    float fftbuf[2*NN];
    float ifftbuf[2*NN];
    float ps[NN];
    float sigpow[NN];
    */
    // printf("ok1 /n");
    // float noise[NN];
    // memcpy(&sub->sbuf[0], &sub->sbuf[NN], NN * sizeof(short));
    // memcpy(&sub->sbuf[NN], in, NN * sizeof(short));
    // sub->framecount++;
 
    for (ii = 0; ii<NN; ii++) {
        fftbuf[ii] = sub->sbuf[ii] * hamin[ii];
    }

    for (ii = 0; ii<NN; ii++) {
        fftbuf[ii+NN] = in[ii] * hamin[ii+NN];
    }

    memcpy(&sub->sbuf[0], in, NN * sizeof(short));
    spx_fft(sub->fft_lookup, fftbuf, fftbuf);
    sigpow[0] = fftbuf[0] * fftbuf[0];
    // int xx=0;
    // xx=sqrt(100);
    // ps[0] = sqrtf(sigpow[0]);

    for (ii = 1; ii < NN; ii++) {
        sigpow[ii] = fftbuf[2 * ii - 1] * fftbuf[2 * ii - 1] +
                     fftbuf[2 * ii] * fftbuf[2 * ii];
       // ps[ii] = sqrtf(sigpow[ii]);
    }

    // SNRseg=10*log10(norm(sig,2)^2/norm(noise_mu,2)^2);

    if (sub->framecount < sub->initial_noise_frame) {
        for (ii = 0; ii < NN; ii++) {
            sub->noise[ii] = (sub->noise[ii]) * (sub->framecount) +  sqrtf(sigpow[ii]);
            sub->noise[ii] /= (sub->framecount + 1);
        }
    }
    sub->framecount++;
    float sigpowsum = 0.0;
    float noisepowsum = 0.0;
    float SNRseg = 0.0;
    float floor = 0.0;
    for (ii = 0; ii < NN; ii++) {
        sigpowsum += sigpow[ii];
        noisepowsum += sub->noise[ii] * sub->noise[ii];
    }

    if ((sigpowsum / noisepowsum) > 0) {
        SNRseg = 10 * log10f(sigpowsum / noisepowsum);
    } else {
        SNRseg = 100;
    }
    sub->snr = SNRseg;
    sub->beta = berouti(SNRseg);

    for (ii = 0; ii < NN; ii++) {
        sub->ratio[ii] =
            1 - ((sub->beta * sub->noise[ii] * sub->noise[ii]) / sigpow[ii]);
        floor = (sub->FLOOR * sub->noise[ii] * sub->noise[ii]) / sigpow[ii];
        if (sub->ratio[ii] < floor) {
            sub->ratio[ii] = floor;
        }
    }

    if ((SNRseg < sub->Thres) &&
        (sub->framecount >= sub->initial_noise_frame)) {
        for (ii = 0; ii < NN; ii++) {
            sub->noise[ii] = sqrtf(sub->G * sub->noise[ii] * sub->noise[ii] +
                                   (1 - sub->G) * sigpow[ii]);
        }
    }

    fftbuf[0] *= sqrtf(sub->ratio[0]);
    for (ii = 1; ii < NN; ii++) {
        fftbuf[2 * ii - 1] = fftbuf[2 * ii - 1] * sqrtf(sub->ratio[ii]);
        fftbuf[2 * ii] = fftbuf[2 * ii] * sqrtf(sub->ratio[ii]);
    }
    fftbuf[2 * NN - 1] *= sqrtf(sub->ratio[NN - 1]);
    spx_ifft(sub->fft_lookup, fftbuf, fftbuf);
    for (ii = 0; ii < NN; ii++) {
        in[ii] = (short)(sub->winGain * (sub->overlap[ii] + fftbuf[ii]));
        sub->overlap[ii] = fftbuf[NN + ii];
    }
}
