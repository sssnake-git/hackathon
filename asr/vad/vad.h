/*
Descrption: vad header file

Author:
Last Modify:
Date: 3 Aug 2022
*/

#ifndef __CCREC_VAD_H__
#define __CCREC_VAD_H__

#include <stdbool.h>
#include <string.h>

#if defined(__CCREC_8K_ACMOD__)
#define __CCREC_VAD_RINGBUF_SIZE__                                       \
    (4096 / 2) /* minimum ringbuff size is 2048 samples (128ms for 16khz \
                  sample rate) */
#define __CCREC_VAD_MIN_SPEECH_SAMPLE__                                 \
    480 /* minimum voice duration in samples (60ms) before VAD actually \
deteced the begining of the voice, bigger value less sensitivity */
#define __CCREC_VAD_MIN_TS_SAMPLE__ \
    2400 /* minimum trailing silence in samples (500ms) before EOS decided */
#define __CCREC_VAD_SENSITIVITY__ \
    1.5 /* suggested to be between 1~3 , smaller, more sensitive */
#define __CCREC_VAD_EOS_SENSITIVITY__ \
    0.8 /* suggested to be between 0.5~1, smaller, dummer */
#define __CCREC_VAD_ENG_ABSTH__ \
    100 /* absolute threshold of energy to determin */
#define __CCREC_VAD_WINSIZE_SAMPLE__ \
    240 /* window size in samples (16 * 30ms) assuming sample rate is 16K */
#define __CCREC_VAD_WINSHIFT_SAMPLE__ \
    80 /* window shifting size for vad (16 * 10ms) */

#else

#define __CCREC_VAD_RINGBUF_SIZE__                                          \
    (4096) /* minimum ringbuff size is 2048 samples (128ms for 16khz sample \
              rate) */
#define __CCREC_VAD_MIN_SPEECH_SAMPLE__                                 \
    640 /* minimum voice duration in samples (60ms) before VAD actually \
deteced the begining of the voice, bigger value less sensitivity */
#define __CCREC_VAD_MIN_TS_SAMPLE__ \
    9600 /* minimum trailing silence in samples (1s) before EOS decided */
#define __CCREC_VAD_SENSITIVITY__ \
    1.5 /* suggested to be between 1~3 , smaller, more sensitive */
#define __CCREC_VAD_EOS_SENSITIVITY__ \
    0.6 /* suggested to be between 0.5~1, smaller, dummer */
#define __CCREC_VAD_ENG_ABSTH__ \
    30 /* absolute threshold of energy to determin */
#define __CCREC_VAD_WINSIZE_SAMPLE__ \
    480 /* window size in samples (16 * 30ms) assuming sample rate is 16K */
#define __CCREC_VAD_WINSHIFT_SAMPLE__ \
    160 /* window shifting size for vad (16 * 10ms) */
#endif

typedef struct _ccrec_vad_ring_buf_ {
    short *p_head;
    short *p_tail;
    short *p_valid;
    short *p_valid_tail;
    short valid_len;
} _ccrec_vad_ring_buf_;

typedef struct _ccrec_vad_ {
    bool b_bos_detected; /* true if a start point found */
    bool b_possible_bos; /* true if the first frame matching >
                            sensitivity*favgEng */
    bool b_eos_detected;
    bool b_posssible_eos;

    _ccrec_vad_ring_buf_
        *_p_buf;            /* sample buf to hold a window of speech signal */
    short *buff_in_process; /* processing buffer */
    short *return_buffer;   /* data buffer to return to application, storing the
                               data before BOS to BOS*/

    float f_acc_eng;
    float f_avg_eng; /* average energy from the beginning of the signal to
                        current frame */

    float f_bg_acc_eng;
    float f_bg_avg_eng; /* average energy outside the speech */

    float f_short_time_acc_eng;
    float f_short_time_avg_eng; /* short-time avg energy between possile_bos and
                                   realbos */

    unsigned short p_buf_offset; /* buffer handling */
    unsigned int n_frame_count;  /* accumulated frames until current processing
                                    before VAD detected */
    unsigned short n_short_time_frame_count; /* short-time frames for short-time
                                                avg energy calculation */

    /* Debug */
    unsigned int n_idx; /* frame ID only for debug usage*/
    unsigned int n_bg_idx;

    unsigned int n_ms_det_speech; /* detected speech before determing it is real
                                     VAD point */
    unsigned int n_ms_det_trailing_sil;  /* detected trailing silence before a
                                            real EOS deteded */
    unsigned int n_delayed_writing_smps; /* the delayed samples needed writing
                                            to engine after EOS */

    unsigned int n_min_trailing_sil_smps; /* minimum trailing silence in samples
                                             before EOS decided */
    float f_abs_eng_th;  /* absolute threshold of energy to determin */
    float f_sensitivity; /* suggested to be between 1~3 , smaller, more
                            sensitive */
} _ccrec_vad_;

_ccrec_vad_ *vad_initialize(short nlength);
float vad_calc_energy(short *_p_buf, short data_len);
void vad_reset(_ccrec_vad_ *_p_inited_vad);
short vad_add_wav_data(_ccrec_vad_ *_p_inited_vad, const short *in_data,
                       short in_nSamples, short **out_nSamples,
                       short *isBOS_or_EOS_Buffer);
void vad_release(_ccrec_vad_ *_p_vad);

void vad_set_min_ts_smps(_ccrec_vad_ *_p_inited_vad, unsigned int val);
void vad_set_abs_eng_th(_ccrec_vad_ *_p_inited_vad, float val);
void vad_set_sensitivity(_ccrec_vad_ *_p_inited_vad, float val);

#endif  //#define __CCREC_VAD_H__
