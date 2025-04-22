#ifndef __LIBCNVAD_H__
#define __LIBCNVAD_H__

#ifdef __cplusplus
extern "C" {
#endif

void *vad_initialize(void *asr_handle);

void vad_reset(void *vad_engine);
void reset_vad_all(void *vad_engine);
void vad_release(void *vad_engine);

short vad_add_wav_data(void *vad_engine, short *in_data, short in_nSamples, short **out_nSamples, unsigned int *bos,
        unsigned int *eos, short *isBOS_or_EOS_Buffer);

#ifdef __cplusplus
}
#endif

#endif
