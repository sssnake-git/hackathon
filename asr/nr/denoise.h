#ifndef _DE_NOISE_H_
#define _DE_NOISE_H_



#define NN 256
// #define int16_t short
#define USE_SMALLFT

typedef struct specsub_s {
    float Thres;
    float alpha;
    float FLOOR;
    float G;
    float winGain;
    int initial_noise_frame;
    int framecount;
    void* fft_lookup;
    float beta;
    float snr;
    float overlap[NN];
    float noise[ NN];
    float ratio[NN];
    short sbuf[NN];
} specsub;

void spec_sub_ini(specsub* sub);
void spec_sub_deini(specsub* sub);
float berouti(float snr);
void spec_sub(specsub* sub, short* in);
float get_snr(specsub* sub);

#endif  // _DE_NOISE_H_