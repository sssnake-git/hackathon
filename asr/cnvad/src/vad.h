#ifndef __LIBCNVAD_H__
#define __LIBCNVAD_H__

#ifdef __cplusplus
extern "C" {
#endif

void *vad_initialize(void *asr_handle);

void vad_reset(void *pInst_vad);
void reset_vad_all(void *pInst_vad);
void vad_release(void *pInst_vad);

short vad_add_wav_data(void *pInst_vad, short *in_data, short in_nSamples, short **out_nSamples, unsigned int *bos,
          unsigned int *eos, short *isBOS_or_EOS_Buffer);

#ifdef __cplusplus
}
#endif

#endif
