/*
Descrption: fft header

Author:
Last Modify:
Date: 28 July 2022
*/

#ifndef __CCREC_FEAT_FFT_H__
#define __CCREC_FEAT_FFT_H__

// ----------------------------------------------------------------
// #include "symbol_scramble.h" /* make this the first include! */
// ----------------------------------------------------------------

void ccrec_feat_rfft(float *signal_frame, int frame_len, int fft_len);

#endif
