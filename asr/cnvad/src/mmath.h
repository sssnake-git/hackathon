#ifndef __MMATH_H__
#define __MMATH_H__
typedef union {
    float f;
    uint32_t u;
    int32_t i;
} ufi_t;

#define FM_FLOAT_BIAS 127
#define FM_FLOAT_INIT_EXP(var,num)              \
    var.i = (((int) num) + FM_FLOAT_BIAS) << 23

#define FM_FLOAT_LOG2OFE  1.4426950408889634074f

static const float fm_exp2f_p[] __attribute__ ((aligned(8))) = {
    1.535336188319500e-4f,
    1.339887440266574e-3f,
    9.618437357674640e-3f,
    5.550332471162809e-2f,
    2.402264791363012e-1f,
    6.931472028550421e-1f,
    1.000000000000000f
};

static inline float fm_exp2f(float x) {
    float ipart, fpart;
    ufi_t epart;

    // ipart = __builtin_floorf(x + 0.5f);
    ipart = (int)(x + 0.5f);
    fpart = x - ipart;
    FM_FLOAT_INIT_EXP(epart,ipart);

    x = fm_exp2f_p[0];
    x = x * fpart + fm_exp2f_p[1];
    x = x * fpart + fm_exp2f_p[2];
    x = x * fpart + fm_exp2f_p[3];
    x = x * fpart + fm_exp2f_p[4];
    x = x * fpart + fm_exp2f_p[5];
    x = x * fpart + fm_exp2f_p[6];

    return epart.f * x;
}

static inline float fm_expf(float x) {
    return fm_exp2f(FM_FLOAT_LOG2OFE*x);
}

float fastsqrt(float val) {
    int tmp = *(int *)&val;
    tmp -= 1<<23;
    tmp = tmp >> 1;
    tmp += 1<<29;
    return *(float *)&tmp;
}

#define expf fm_expf
#define sqrt fastsqrt

#endif
