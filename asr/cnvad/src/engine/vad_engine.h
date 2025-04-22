#ifndef __CNVAD_ENGINE__
#define __CNVAD_ENGINE__

#ifdef __cplusplus
extern "C" {
#endif

void vad_engine_init(void** self);
void vad_engine_set_param(void* self, int MinSpeech, int MinTS, int MinVadTh, float gapCoef, int LS, int TS);
int vad_engine_process(void* self, int eng, int *offset);
void vad_engine_set_loglevel(void* self, char* path);
void vad_engine_full_reset(void* self);
void vad_engine_reset(void* self);
void vad_engine_close(void* self);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __CNVAD_ENGINE__
