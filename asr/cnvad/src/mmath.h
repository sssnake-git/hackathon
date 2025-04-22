#ifndef __MMATH_H__
#define __MMATH_H__
#if 0
#define N (1 << EXP2F_TABLE_BITS)
#define InvLn2N __exp2f_data.invln2_scaled
#define T __exp2f_data.tab
#define C __exp2f_data.poly_scaled

static inline uint32_t top12 (float x) {
    return (*(uint32_t*)(&x)) >> 20;
}

static inline float mexpf (float x) {
    uint32_t abstop;
    uint64_t ki, t;
    /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
    double_t kd, xd, z, r, r2, y, s;
    xd = (double_t) x;
    abstop = top12 (x) & 0x7ff;
    if (__glibc_unlikely (abstop >= top12 (88.0f)))
    {
        /* |x| >= 88 or x is nan.  */
        if (asuint (x) == asuint (-INFINITY))
            return 0.0f;
        if (abstop >= top12 (INFINITY))
            return x + x;
        if (x > 0x1.62e42ep6f) /* x > log(0x1p128) ~= 88.72 */
            return __math_oflowf (0);
        if (x < -0x1.9fe368p6f) /* x < log(0x1p-150) ~= -103.97 */
            return __math_uflowf (0);
#if WANT_ERRNO_UFLOW
        if (x < -0x1.9d1d9ep6f) /* x < log(0x1p-149) ~= -103.28 */
            return __math_may_uflowf (0);
#endif
    }
    /* x*N/Ln2 = k + r with r in [-1/2, 1/2] and int k.  */
    z = InvLn2N * xd;
    /* Round and convert z to int, the result is in [-150*N, 128*N] and
       ideally ties-to-even rule is used, otherwise the magnitude of r
       can be bigger which gives larger approximation error.  */
#if TOINT_INTRINSICS
    kd = roundtoint (z);
    ki = converttoint (z);
#else
# define SHIFT __exp2f_data.shift
    kd = math_narrow_eval ((double) (z + SHIFT)); /* Needs to be double.  */
    ki = asuint64 (kd);
    kd -= SHIFT;
#endif
    r = z - kd;
    /* exp(x) = 2^(k/N) * 2^(r/N) ~= s * (C0*r^3 + C1*r^2 + C2*r + 1) */
    t = T[ki % N];
    t += ki << (52 - EXP2F_TABLE_BITS);
    s = asdouble (t);
    z = C[0] * r + C[1];
    r2 = r * r;
    y = C[2] * r + 1;
    y = z * r2 + y;
    y = y * s;
    return (float) y;
}
#else

typedef union 
{
    float    f;
    uint32_t u;
    int32_t  i;
}  ufi_t;

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

    x =           fm_exp2f_p[0];
    x = x*fpart + fm_exp2f_p[1];
    x = x*fpart + fm_exp2f_p[2];
    x = x*fpart + fm_exp2f_p[3];
    x = x*fpart + fm_exp2f_p[4];
    x = x*fpart + fm_exp2f_p[5];
    x = x*fpart + fm_exp2f_p[6];

    return epart.f*x;
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
#endif
