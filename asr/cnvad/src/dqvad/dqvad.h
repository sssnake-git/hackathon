#ifndef __CNVAD_ENGINE__
#define __CNVAD_ENGINE__

#ifdef __cplusplus
extern "C" {
#endif

void cnvad_engine_init(void** self);
void cnvad_engine_set_param(void* self, int MinSpeech, int MinTS, int MinVadTh, float gapCoef, int LS, int TS);
int cnvad_engine_process(void* self, int eng, int *offset);
void DqVad_SetLogOut(void* self, char* path);
void DqVad_FullReset(void* self);
void DqVad_Reset(void* self);
void DqVad_Close(void* self);

#ifdef __cplusplus
}
#endif

#endif // #ifndef __CNVAD_ENGINE__
