#include <stdint.h>
// #include <math.h>
#include <stdio.h>
#include <string.h>
#include "mmath.h"
#include "ccnn_core.h"
typedef int intsum_t;

#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#define limit16(x) max(min(x, 32767), -32768)
#define limit32(x) max(min(x, 2147483647), -2147483648)
#define getaddr(memtab, addrid) \
  ((uint8_t*)memtab[addrid>>24] + (addrid&0xFFFFFF))
#define getaddr0(tab, addrid) \
  ((uint8_t*)memtab[0] + addrid)

int ccnn_execute(void **memtab, void* op, int opsize) {
    uint8_t *opptr = (uint8_t*)op;
    uint8_t *opend = opptr + opsize;

    int i, j;
    int opi = 0;

    while (opptr < opend) {
        int op_type = *(uint16_t *)opptr;
        switch (op_type) {
#if defined HAS_ALL_OP || defined HAS_OP_MATMUL
            case OP_MATMUL: {
                // printf("matmul\n");
                CCNR_OP_MAT* opmat = (CCNR_OP_MAT*)opptr;
                const float *input = (float*)getaddr(memtab, opmat->input);
                float *output = (float*)getaddr(memtab, opmat->output);
                const float *w = (float*)getaddr0(memtab, opmat->weight);
                for (i=0; i<opmat->col; i++) {
                    float tmp = 0;
                    for (j=0; j<opmat->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp;
                    w += opmat->row;
                }
                opptr += sizeof(CCNR_OP_MAT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMUL
            case OP_IMATMUL: {
                // printf("imatmul\n");
                CCNR_OP_MAT* opmat = (CCNR_OP_MAT*)opptr;
                const int* input = (int*)getaddr(memtab, opmat->input);
                float *output = (float*)getaddr(memtab, opmat->output);
                const float *w = (float*)getaddr0(memtab, opmat->weight);
                for (i=0; i<opmat->col; i++) {
                    float tmp = 0;
                    for (j=0; j<opmat->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp;
                    w += opmat->row;
                }
                opptr += sizeof(CCNR_OP_MAT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8
            case OP_IMATMULI8: {
                // printf("imatmuli8\n");
                CCNR_OP_MAT* opmat = (CCNR_OP_MAT*)opptr;
                const int* input = (int*)getaddr(memtab, opmat->input);
                int* output = (int*)getaddr(memtab, opmat->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmat->weight);
                for (i=0; i<opmat->col; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<opmat->row; j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    output[i] = tmp;
                    w += opmat->row;
                }
                opptr += sizeof(CCNR_OP_MAT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MATMULI8Q
            case OP_MATMULI8Q: {
                // printf("matmuli8q\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const float *input = (float*)getaddr(memtab, opmatq->input);
                float *output = (float*)getaddr(memtab, opmatq->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    float tmp = 0;
                    for (j=0; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8Q
            case OP_IMATMULI8Q: {
                // printf("imatmuli8q\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int *input = (int*)getaddr(memtab, opmatq->input);
                float *output = (float*)getaddr(memtab, opmatq->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<(opmatq->row&(~3)); j+=4) {
                        tmp += input[j+0]*w[j+0];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    for (; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8QI
            case OP_IMATMULI8QI: {
                // printf("imatmuli8qi\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int *input = (int*)getaddr(memtab, opmatq->input);
                int *output = (int*)getaddr(memtab, opmatq->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    int64_t tmp = 0;
                    for (j=0; j<(opmatq->row&(~3)); j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    for (; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8SI
            case OP_IMATMULI8SI: {
                // printf("imatmuli8si\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int *input = (int*)getaddr(memtab, opmatq->input);
                int *output = (int*)getaddr(memtab, opmatq->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmatq->weight);
                int shift = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<(opmatq->row&(~3)); j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    for (; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp>>shift;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16MATMULI8Q
            case OP_I16MATMULI8Q: {
                // printf("i16matmuli8q\n", quant);
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int16_t *input = (int16_t*)getaddr(memtab, opmatq->input);
                float *output = (float*)getaddr(memtab, opmatq->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<opmatq->row; j+=4) {
                        tmp += input[j+0]*w[j+0];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16MATMULI8QI16
            case OP_I16MATMULI8QI16: {
                // printf("i16matmuli8qi16\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int16_t *input = (int16_t*)getaddr(memtab, opmatq->input);
                int16_t *output = (int16_t*)getaddr(memtab, opmatq->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<(opmatq->row&(~3)); j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    for (; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = limit16(tmp*quant);
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MATMULI16Q
            case OP_MATMULI16Q: {
                // printf("matmuli16q\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const float *input = (float*)getaddr(memtab, opmatq->input);
                float *output = (float*)getaddr(memtab, opmatq->output);
                const int16_t *w = (int16_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    float tmp = 0;
                    for (j=0; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI16Q
            case OP_IMATMULI16Q: {
                // printf("imatmuli16q\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int *input = (int*)getaddr(memtab, opmatq->input);
                float *output = (float*)getaddr(memtab, opmatq->output);
                const int16_t *w = (int16_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<opmatq->row; j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI16I
            case OP_IMATMULI16I: {
                CCNR_OP_MAT* opmatq = (CCNR_OP_MAT*)opptr;
                const int *input = (int*)getaddr(memtab, opmatq->input);
                int *output = (int*)getaddr(memtab, opmatq->output);
                const int16_t *w = (int16_t*)getaddr0(memtab, opmatq->weight);
                for (i=0; i<opmatq->col; i++) {
                    int64_t tmp = 0;
                    for (j=0; j<(opmatq->row&(~3)); j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    for (; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MAT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI16QI
            case OP_IMATMULI16QI: {
                // printf("imatmuli16qi\n");
                CCNR_OP_MATQ* opmatq = (CCNR_OP_MATQ*)opptr;
                const int *input = (int*)getaddr(memtab, opmatq->input);
                int *output = (int*)getaddr(memtab, opmatq->output);
                const int16_t *w = (int16_t*)getaddr0(memtab, opmatq->weight);
                float quant = opmatq->quant;
                for (i=0; i<opmatq->col; i++) {
                    int64_t tmp = 0;
                    for (j=0; j<(opmatq->row&(~3)); j+=4) {
                        tmp += input[j]*w[j];
                        tmp += input[j+1]*w[j+1];
                        tmp += input[j+2]*w[j+2];
                        tmp += input[j+3]*w[j+3];
                    }
                    for (; j<opmatq->row; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp*quant;
                    w += opmatq->row;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ILMULI8I
            case OP_ILMULI8I: {
                // printf("ilmuli8i\n");
                CCNR_OP_MAT* opobj = (CCNR_OP_MAT*)opptr;
                const int* input = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                for (i=0; i<opobj->row; i++) {
                    int64_t tmp = 0;
                    for (j=0; j<opobj->col; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp;
                    input += opobj->col;
                }
                opptr += sizeof(CCNR_OP_MAT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ILMULI8QI
            case OP_ILMULI8QI: {
                // printf("ilmuli8qi\n");
                CCNR_OP_MATQ* opobj = (CCNR_OP_MATQ*)opptr;
                const int* input = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                float quant = opobj->quant;
                for (i=0; i<opobj->row; i++) {
                    int64_t tmp = 0;
                    for (j=0; j<opobj->col; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp*quant;
                    input += opobj->col;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ILMULI8SI
            case OP_ILMULI8SI: {
                // printf("ilmuli8si\n");
                CCNR_OP_MATQ* opobj = (CCNR_OP_MATQ*)opptr;
                const int* input = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                int shift = opobj->quant;
                for (i=0; i<opobj->row; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<opobj->col; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = tmp>>shift;
                    input += opobj->col;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16LMULI8I16
            case OP_I16LMULI8I16: {
                // printf("i16lmuli8i16\n");
                CCNR_OP_MAT* opobj = (CCNR_OP_MAT*)opptr;
                const int16_t* input = (int16_t*)getaddr(memtab, opobj->input);
                int16_t *output = (int16_t*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                for (i=0; i<opobj->row; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<opobj->col; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = limit16(tmp);
                    input += opobj->col;
                }
                opptr += sizeof(CCNR_OP_MAT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16LMULI8QI16
            case OP_I16LMULI8QI16: {
                // printf("i16lmuli8i16\n");
                CCNR_OP_MATQ* opobj = (CCNR_OP_MATQ*)opptr;
                const int16_t* input = (int16_t*)getaddr(memtab, opobj->input);
                int16_t *output = (int16_t*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                float quant = opobj->quant;
                for (i=0; i<opobj->row; i++) {
                    intsum_t tmp = 0;
                    for (j=0; j<opobj->col; j++) {
                        tmp += input[j]*w[j];
                    }
                    output[i] = limit16(tmp*quant);
                    input += opobj->col;
                }
                opptr += sizeof(CCNR_OP_MATQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ICONV2DI8QI
            case OP_ICONV2DI8QI: {
                // printf("i16conv2di8qi16\n");
                CCNR_OP_CONV2DQ* opobj = (CCNR_OP_CONV2DQ*)opptr;
                const int *inbase = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *wbase = (int8_t*)getaddr0(memtab, opobj->weight);
                float quant = opobj->quant;
                int m, n, k;
                int ncol = opobj->col - opobj->kcol + 1;
                int nrow = opobj->row - opobj->krow + 1;
                for (m=0; m<ncol; m++) {
                    for (n=0; n<nrow; n++) {
                        for (k=0; k<opobj->nfilter; k++) {
                            int64_t tmp = 0;
                            const int8_t *w = wbase+opobj->kcol*opobj->krow*k;
                            const int *input = inbase+opobj->row*m+n;
                            for (i=0; i<opobj->kcol; i++) {
                                for (j=0; j<(opobj->krow&(~3)); j+=4) {
                                    tmp += input[j]*w[j];
                                    tmp += input[j+1]*w[j+1];
                                    tmp += input[j+2]*w[j+2];
                                    tmp += input[j+3]*w[j+3];
                                }
                                // for (; j<opobj->krow; j++) {
                                //     tmp += input[j]*w[j];
                                // }
                                w += opobj->krow;
                                input += opobj->row;
                            }
                            output[k] = tmp*quant;
                        }
                        output += opobj->nfilter;
                    }
                }
                opptr += sizeof(CCNR_OP_CONV2DQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16CONV2DI8QI16
            case OP_I16CONV2DI8QI16: {
                // printf("i16conv2di8qi16\n");
                CCNR_OP_CONV2DQ* opobj = (CCNR_OP_CONV2DQ*)opptr;
                const int16_t *inbase = (int16_t*)getaddr(memtab, opobj->input);
                int16_t *output = (int16_t*)getaddr(memtab, opobj->output);
                const int8_t *wbase = (int8_t*)getaddr0(memtab, opobj->weight);
                float quant = opobj->quant;
                int m, n, k;
                int ncol = opobj->col - opobj->kcol + 1;
                int nrow = opobj->row - opobj->krow + 1;
                for (m=0; m<ncol; m++) {
                    for (n=0; n<nrow; n++) {
                        for (k=0; k<opobj->nfilter; k++) {
                            intsum_t tmp = 0;
                            const int8_t *w = wbase+opobj->kcol*opobj->krow*k;
                            const int16_t *input = inbase+opobj->row*m+n;
                            for (i=0; i<opobj->kcol; i++) {
                                for (j=0; j<(opobj->krow&(~3)); j+=4) {
                                    tmp += input[j]*w[j];
                                    tmp += input[j+1]*w[j+1];
                                    tmp += input[j+2]*w[j+2];
                                    tmp += input[j+3]*w[j+3];
                                }
                                // for (; j<opobj->krow; j++) {
                                //     tmp += input[j]*w[j];
                                // }
                                w += opobj->krow;
                                input += opobj->row;
                            }
                            output[k] = limit16(tmp*quant);
                        }
                        output += opobj->nfilter;
                    }
                }
                opptr += sizeof(CCNR_OP_CONV2DQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IPCONVDI8I
            case OP_IPCONVDI8I: {
                CCNR_OP_PCONV* opobj = (CCNR_OP_PCONV*)opptr;
                int *input = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                int datalen = opobj->row;
                int convlen = opobj->col;
                int halflen = convlen>>1;
                int n;
                memset(input, 0, halflen*sizeof(int));
                memset(input+datalen+halflen, 0, (convlen-halflen-1)*sizeof(int));
                for (i=0; i<datalen; i++) {
                    intsum_t tmp = 0;
                    const int *ipos = input+i;
                    for (n=0; n<opobj->nfilter; n++) {
                        for (j=0; j<(convlen&(~3)); j+=4) {
                            tmp += ipos[j]*w[j];
                            tmp += ipos[j+1]*w[j+1];
                            tmp += ipos[j+2]*w[j+2];
                            tmp += ipos[j+3]*w[j+3];
                        }
                        for (; j<convlen; j++) {
                            tmp += ipos[j]*w[j];
                        }
                        *output++ = tmp;
                        w += convlen;
                    }
                }
                opptr += sizeof(CCNR_OP_PCONV);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16PCONVDI8I16
            case OP_I16PCONVDI8I16: {
                CCNR_OP_PCONV* opobj = (CCNR_OP_PCONV*)opptr;
                int16_t *input = (int16_t*)getaddr(memtab, opobj->input);
                int16_t *output = (int16_t*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                int datalen = opobj->row;
                int convlen = opobj->col;
                int halflen = convlen>>1;
                int n;
                memset(input, 0, halflen*sizeof(int16_t));
                memset(input+datalen+halflen, 0, (convlen-halflen-1)*sizeof(int16_t));
                for (i=0; i<datalen; i++) {
                    intsum_t tmp = 0;
                    const int16_t *ipos = input+i;
                    for (n=0; n<opobj->nfilter; n++) {
                        for (j=0; j<(convlen&(~3)); j+=4) {
                            tmp += ipos[j]*w[j];
                            tmp += ipos[j+1]*w[j+1];
                            tmp += ipos[j+2]*w[j+2];
                            tmp += ipos[j+3]*w[j+3];
                        }
                        for (; j<convlen; j++) {
                            tmp += ipos[j]*w[j];
                        }
                        *output++ = tmp;
                        w += convlen;
                    }
                }
                opptr += sizeof(CCNR_OP_PCONV);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IPCONVTI8I
            case OP_IPCONVTI8I: {
                CCNR_OP_PCONV* opobj = (CCNR_OP_PCONV*)opptr;
                int *input = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                int datalen = opobj->row;
                int convlen = opobj->col;
                int n;
                for (i=0; i<datalen; i++) {
                    const int *ipos = input+i;
                    for (n=0; n<opobj->nfilter; n++) {
                        intsum_t tmp = 0;
                        for (j=0; j<(convlen&(~3)); j+=4) {
                            tmp += ipos[0]*w[j];
                            tmp += ipos[datalen]*w[j+1];
                            tmp += ipos[datalen*2]*w[j+2];
                            tmp += ipos[datalen*3]*w[j+3];
                            ipos += datalen*4;
                        }
                        ipos = input+i;
                        for (; j<convlen; j++) {
                            tmp += ipos[datalen*j]*w[j];
                        }
                        *output++ = tmp;
                        w += convlen;
                    }
                }
                memmove(input, input+datalen, datalen*(convlen-1)*sizeof(int));
                opptr += sizeof(CCNR_OP_PCONV);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16PCONVTI8I16
            case OP_I16PCONVTI8I16: {
                CCNR_OP_PCONV* opobj = (CCNR_OP_PCONV*)opptr;
                int16_t *input = (int16_t*)getaddr(memtab, opobj->input);
                int16_t *output = (int16_t*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                int datalen = opobj->row;
                int convlen = opobj->col;
                int n;
                for (i=0; i<datalen; i++) {
                    intsum_t tmp = 0;
                    const int16_t *ipos = input+i;
                    for (n=0; n<opobj->nfilter; n++) {
                        for (j=0; j<(convlen&(~3)); j+=4) {
                            tmp += ipos[0]*w[j];
                            tmp += ipos[datalen]*w[j+1];
                            tmp += ipos[datalen*2]*w[j+2];
                            tmp += ipos[datalen*3]*w[j+3];
                        }
                        for (; j<convlen; j++) {
                            tmp += ipos[datalen*j]*w[j];
                        }
                        *output++ = limit16(tmp);
                        w += convlen;
                    }
                }
                memmove(input, input+datalen, datalen*(convlen-1)*sizeof(int16_t));
                opptr += sizeof(CCNR_OP_PCONV);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IPCONVTI8QI
            case OP_IPCONVTI8QI: {
                CCNR_OP_PCONV* opobj = (CCNR_OP_PCONV*)opptr;
                int *input = (int*)getaddr(memtab, opobj->input);
                int *output = (int*)getaddr(memtab, opobj->output);
                const int8_t *w = (int8_t*)getaddr0(memtab, opobj->weight);
                float quant = opobj->quant;
                int datalen = opobj->row;
                int convlen = opobj->col;
                int n;
                for (i=0; i<datalen; i++) {
                    const int *ipos = input+i;
                    for (n=0; n<opobj->nfilter; n++) {
                        int64_t tmp = 0;
                        for (j=0; j<(convlen&(~3)); j+=4) {
                            tmp += ipos[0]*w[j];
                            tmp += ipos[datalen]*w[j+1];
                            tmp += ipos[datalen*2]*w[j+2];
                            tmp += ipos[datalen*3]*w[j+3];
                            ipos += datalen*4;
                        }
                        ipos = input+i;
                        for (; j<convlen; j++) {
                            tmp += ipos[datalen*j]*w[j];
                        }
                        *output++ = tmp*quant;
                        w += convlen;
                    }
                }
                memmove(input, input+datalen, datalen*(convlen-1)*sizeof(int));
                opptr += sizeof(CCNR_OP_PCONV);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MUL
            case OP_MUL: {
                // printf("mul\n");
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]*w[i];
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMULQ
            case OP_IMULQ: {
                // printf("imulq\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]*w[i]*quant;
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16MULQ
            case OP_I16MULQ: {
                // printf("i16mulq\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const int16_t *input = (int16_t*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]*w[i]*quant;
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MULQI
            case OP_MULQI: {
                // printf("mulqi\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]*w[i]*quant;
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MULQI16
            case OP_MULQI16: {
                // printf("mulqi16\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int16_t *output = (int16_t*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = limit16(input[i]*w[i]*quant);
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMULQI
            case OP_IMULQI: {
                // printf("imulqi\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]*w[i]*quant;
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ADD
            case OP_ADD: {
                // printf("add\n");
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]+w[i];
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IADDI
            case OP_IADDI: {
                // printf("iaddi\n");
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const int *w = (int*)getaddr(memtab, opa->weight);
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i]+w[i];
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IADDI8M
            case OP_IADDI8M: {
                // printf("iaddi8m\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                const int8_t *w = (int8_t*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = (input[i]+w[i])*quant;
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ADDMI
            case OP_ADDMI: {
                // printf("addmi\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = (input[i]+w[i])*quant;
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ADDMI16
            case OP_ADDMI16: {
                // printf("addmi16\n");
                CCNR_OP_ARRQ* opa = (CCNR_OP_ARRQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int16_t *output = (int16_t*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr(memtab, opa->weight);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = limit16((input[i]+w[i])*quant);
                }
                opptr += sizeof(CCNR_OP_ARRQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CSUB
            case OP_CSUB: {
                // printf("csub\n");
                CCNR_OP_CONS* opa = (CCNR_OP_CONS*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                float alpha = opa->alpha;
                for (i=0; i<opa->row; i++) {
                    output[i] = alpha - input[i];
                }
                opptr += sizeof(CCNR_OP_CONS);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CMUL
            case OP_CMUL: {
                // printf("cmul\n");
                CCNR_OP_CONS* opa = (CCNR_OP_CONS*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                float alpha = opa->alpha;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] * alpha;
                }
                opptr += sizeof(CCNR_OP_CONS);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CIMUL
            case OP_CIMUL: {
                // printf("cimul\n");
                CCNR_OP_CONS* opa = (CCNR_OP_CONS*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                float alpha = opa->alpha;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] * alpha;
                }
                opptr += sizeof(CCNR_OP_CONS);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CMULI
            case OP_CMULI: {
                // printf("cmuli\n");
                CCNR_OP_CONS* opa = (CCNR_OP_CONS*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                float alpha = opa->alpha;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] * alpha;
                }
                opptr += sizeof(CCNR_OP_CONS);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CIMULI
            case OP_CIMULI: {
                // printf("cimuli\n");
                CCNR_OP_CONS* opa = (CCNR_OP_CONS*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                float alpha = opa->alpha;
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] * alpha;
                }
                opptr += sizeof(CCNR_OP_CONS);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CMULI16
            case OP_CMULI16: {
                // printf("cmuli16\n");
                CCNR_OP_CONS* opa = (CCNR_OP_CONS*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int16_t *output = (int16_t*)getaddr(memtab, opa->output);
                float alpha = opa->alpha;
                for (i=0; i<opa->row; i++) {
                    output[i] = limit16(input[i] * alpha);
                }
                opptr += sizeof(CCNR_OP_CONS);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM
            case OP_NORM: {
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                const float *w = (float*)getaddr0(memtab, opa->weight);
                int n = opa->row;
                const float* beta = w + n;
                float avg, std_inv;
                float s;
                for (i = 0, s = 0.0; i < n; ++i) s += input[i];
                avg = (float)(s / n);
                for (i = 0, s = 0.0; i < n; ++i) {
                    output[i] = input[i] - avg;
                    s += output[i] * output[i];
                }
                float epsilon = 1e-7;
                std_inv = (float)(1.0 / sqrt((s / n) + epsilon));
                for (i = 0; i < n; ++i) {
                    output[i] = output[i]*std_inv*w[i] + beta[i];
                }

                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
    #endif
#if defined HAS_ALL_OP || defined HAS_OP_NORMI
            case OP_NORMI: {
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const int16_t *w = (int16_t*)getaddr0(memtab, opa->weight);
                int n = opa->row;
                const int16_t *beta = w + n;
                int64_t s;
                for (i = 0, s = 0; i < n; ++i)
                    s += input[i];
                int avg = s / n;
                for (i = 0, s = 0; i < n; ++i) {
                    int io = input[i] - avg;
                    output[i] = io;
                    s += (int64_t)io * io;
                }
                float epsilon = 1e-7;
                int std = sqrt((s / n) + epsilon);
                std = std ? std : 1;
                for (i = 0; i < n; ++i) {
                    output[i] = (int)((int64_t)output[i]*w[i]/std) + beta[i];
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM16I16
            case OP_NORM16I16: {
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const int16_t *w = (int16_t*)getaddr0(memtab, opa->weight);
                int n = opa->row;
                const int16_t *beta = w + n;
                int64_t s;
                for (i = 0, s = 0; i < n; ++i)
                    s += input[i];
                int avg = s / n;
                for (i = 0, s = 0; i < n; ++i) {
                    int io = input[i] - avg;
                    output[i] = io;
                    s += (int64_t)io * io;
                }
                float epsilon = 1e-7;
                int std = sqrt((s / n) + epsilon);
                std = std ? std : 1;
                int16_t *outputi16 = (int16_t *)output;
                for (i = 0; i < n; ++i) {
                    outputi16[i] = limit16(((int64_t)output[i]*w[i]/std) + beta[i]);
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM8UI
            case OP_NORM8UI: {
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const uint8_t *w = (uint8_t*)getaddr0(memtab, opa->weight);
                int n = opa->row;
                const int8_t *beta = (int8_t *)w + n;
                int64_t s;
                for (i = 0, s = 0; i < n; ++i)
                    s += input[i];
                int avg = s / n;
                for (i = 0, s = 0; i < n; ++i) {
                    int io = input[i] - avg;
                    output[i] = io;
                    s += (int64_t)io * io;
                }
                float epsilon = 1e-7;
                int std = sqrt((s / n) + epsilon);
                std = std ? std : 1;
                for (i = 0; i < n; ++i) {
                    output[i] = (int)((int64_t)output[i]*w[i]/std) + (int)beta[i];
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM8UI16
            case OP_NORM8UI16: {
                CCNR_OP_ARR* opa = (CCNR_OP_ARR*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                const uint8_t *w = (uint8_t*)getaddr0(memtab, opa->weight);
                int n = opa->row;
                const int8_t *beta = (int8_t *)w + n;
                int64_t s;
                for (i = 0, s = 0; i < n; ++i)
                    s += input[i];
                int avg = s / n;
                for (i = 0, s = 0; i < n; ++i) {
                    int io = input[i] - avg;
                    output[i] = io;
                    s += (int64_t)io * io;
                }
                // int64_t avg_eng = (s / n) + 1;
                // float engf = (int)(avg_eng>>32)*(4294967296.0f) + (uint32_t)avg_eng;
                // int std = sqrt(engf);
                float epsilon = 1e-7;
                int std = sqrt((s / n) + epsilon);
                std = std ? std : 1;
                int16_t *outputi16 = (int16_t *)output;
                for (i = 0; i < n; ++i) {
                    outputi16[i] = limit16(((int64_t)output[i]*w[i]/std) + beta[i]);
                }
                opptr += sizeof(CCNR_OP_ARR);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SIGMOID
            case OP_SIGMOID: {
                // printf("sigm\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                for (i=0; i<opa->row; i++) {
                    output[i] = 1.0f / (1.0f + expf(-input[i]));
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_TANH
            case OP_TANH: {
                // printf("tanh\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                for (i=0; i<opa->row; i++) {
                    if (input[i] < -20.0f) {
                        output[i] = -1.0f;
                    } else {
                        float y = expf(-2.0f * input[i]);
                        output[i] = (1.0f - y) / (1.0f + y);
                    }
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SSIGN
            case OP_SSIGN: {
                // printf("softsign\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                for (i=0; i<opa->row; i++) {
                    float inv = input[i];
                    float inabs = inv >=0 ? inv : -inv;
                    output[i] = inv / (1.0f + inabs);
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_RELU
            case OP_RELU: {
                // printf("relu\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] > 0.0f ? input[i] : 0.0f;
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IRELU
            case OP_IRELU: {
                // printf("irelu\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] > 0 ? input[i] : 0;
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ELU
            case OP_ELU: {
                // printf("elu\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                float *output = (float*)getaddr(memtab, opa->output);
                for (i=0; i<opa->row; i++) {
                    output[i] = input[i] > 0.0f? input[i] : (expf(input[i])-1.0f);
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SSIGNQI
            case OP_SSIGNQI: {
                // printf("softsign\n");
                CCNR_OP_ACTQ* opa = (CCNR_OP_ACTQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    float inv = input[i];
                    float inabs = inv >=0 ? inv : -inv;
                    output[i] = inv*quant / (1.0f + inabs);
                }
                opptr += sizeof(CCNR_OP_ACTQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ELUQI
            case OP_ELUQI: {
                // printf("eluqi\n");
                CCNR_OP_ACTQ* opa = (CCNR_OP_ACTQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    output[i] = (input[i] > 0.0f? input[i] : (expf(input[i])-1.0f))*quant;
                }
                opptr += sizeof(CCNR_OP_ACTQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SOFTELQI
            case OP_SOFTELQI: {
                // printf("softelqi\n");
                CCNR_OP_ACTQ* opa = (CCNR_OP_ACTQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                float quant = opa->quant;
                float quant2 = quant*2;
                for (i=0; i<opa->row; i++) {
                    float inv = input[i];
                    float inabs = inv >=0 ? inv : -inv;
                    output[i] = inv*quant2 / (1.0f + inabs) + quant;
                }
                opptr += sizeof(CCNR_OP_ACTQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SOFTELQI16
            case OP_SOFTELQI16: {
                // printf("softelqi\n");
                CCNR_OP_ACTQ* opa = (CCNR_OP_ACTQ*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int16_t *output = (int16_t*)getaddr(memtab, opa->output);
                float quant = opa->quant;
                float quant2 = quant*2;
                for (i=0; i<opa->row; i++) {
                    float inv = input[i];
                    float inabs = inv >=0 ? inv : -inv;
                    output[i] = limit16(inv*quant2 / (1.0f + inabs) + quant);
                }
                opptr += sizeof(CCNR_OP_ACTQ);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_LINEAR
            case OP_LINEAR: {
                // printf("linear\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                int *output = (int*)getaddr(memtab, opa->output);
                // for (i=0; i<opa->row; i++) {
                //     output[i] = input[i];
                // }
                if (opa->input != opa->output) {
                    memcpy(output, input, opa->row*sizeof(float));
                }
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_PRINT
            case OP_PRINT: {
                // printf("print\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const float *input = (float*)getaddr(memtab, opa->input);
                for (i=0; i<opa->row; i++) {
                    printf("%f ", ((float)input[i]));
                }
                printf("\n");
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_PRINTI
            case OP_PRINTI: {
                // printf("printi\n");
                CCNR_OP_ACT* opa = (CCNR_OP_ACT*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                for (i=0; i<opa->row; i++) {
                    printf("%d ", input[i]);
                }
                printf("\n");
                opptr += sizeof(CCNR_OP_ACT);
                break;
            }
#endif
#if defined HAS_ALL_OP || defined HAS_OP_PRINTQI
            case OP_PRINTQI: {
                // printf("printqi\n");
                CCNR_OP_ACTQ* opa = (CCNR_OP_ACTQ*)opptr;
                const int *input = (int*)getaddr(memtab, opa->input);
                float quant = opa->quant;
                for (i=0; i<opa->row; i++) {
                    printf("%.3f ", ((float)input[i])*quant);
                }
                printf("\n");
                opptr += sizeof(CCNR_OP_ACTQ);
                break;
            }
#endif
            default: {
                printf("ccnn_execute op error!!\n");
                return -1;
            }
        }
        opi++;
    }
    return 0;
}
