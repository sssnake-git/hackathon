#ifndef __CCNR_CORE_H__
#define __CCNR_CORE_H__
#include <stdint.h>
// #define HAS_ALL_OP
#include "project_config.h"
#include MDL_OP_CONF_H

enum CCNR_OP_TYPE {
#if defined HAS_ALL_OP || defined HAS_OP_MATMUL
    OP_MATMUL,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMUL
    OP_IMATMUL,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MATMULI8Q
    OP_MATMULI8Q,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8
    OP_IMATMULI8,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8Q
    OP_IMATMULI8Q,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8QI
    OP_IMATMULI8QI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI8SI
    OP_IMATMULI8SI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16MATMULI8Q
    OP_I16MATMULI8Q,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16MATMULI8QI16
    OP_I16MATMULI8QI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MATMULI16Q
    OP_MATMULI16Q,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI16Q
    OP_IMATMULI16Q,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI16I
    OP_IMATMULI16I,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMATMULI16QI
    OP_IMATMULI16QI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ILMULI8I
    OP_ILMULI8I,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ILMULI8QI
    OP_ILMULI8QI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ILMULI8SI
    OP_ILMULI8SI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16LMULI8I16
    OP_I16LMULI8I16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16LMULI8QI16
    OP_I16LMULI8QI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ICONV2DI8QI
    OP_ICONV2DI8QI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16CONV2DI8QI16
    OP_I16CONV2DI8QI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IPCONVDI8I
    OP_IPCONVDI8I,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16PCONVDI8I16
    OP_I16PCONVDI8I16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IPCONVTI8I
    OP_IPCONVTI8I,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16PCONVTI8I16
    OP_I16PCONVTI8I16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IPCONVTI8QI
    OP_IPCONVTI8QI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MUL
    OP_MUL,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMULQ
    OP_IMULQ,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_I16MULQ
    OP_I16MULQ,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MULQI
    OP_MULQI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_MULQI16
    OP_MULQI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IMULQI
    OP_IMULQI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ADD
    OP_ADD,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IADDI
    OP_IADDI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IADDI8M
    OP_IADDI8M,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ADDMI
    OP_ADDMI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ADDMI16
    OP_ADDMI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CSUB
    OP_CSUB,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CMUL
    OP_CMUL,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CIMUL
    OP_CIMUL,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CMULI
    OP_CMULI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CIMULI
    OP_CIMULI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_CMULI16
    OP_CMULI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM
    OP_NORM,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORMI
    OP_NORMI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM16I16
    OP_NORM16I16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM8UI
    OP_NORM8UI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_NORM8UI16
    OP_NORM8UI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SIGMOID
    OP_SIGMOID,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_TANH
    OP_TANH,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SSIGN
    OP_SSIGN,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SSIGNQI
    OP_SSIGNQI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_RELU
    OP_RELU,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_IRELU
    OP_IRELU,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ELU
    OP_ELU,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_ELUQI
    OP_ELUQI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SOFTELQI
    OP_SOFTELQI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_SOFTELQI16
    OP_SOFTELQI16,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_LINEAR
    OP_LINEAR,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_PRINT
    OP_PRINT,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_PRINTI
    OP_PRINTI,
#endif
#if defined HAS_ALL_OP || defined HAS_OP_PRINTQI
    OP_PRINTQI,
#endif
};

typedef struct CCNR_OP_MAT {
    uint16_t op_type;
    uint16_t row;
    uint16_t col;
    uint32_t weight;
    uint32_t input;
    uint32_t output;
} CCNR_OP_MAT;

typedef struct CCNR_OP_MATQ {
    uint16_t op_type;
    uint16_t row;
    uint16_t col;
    uint32_t weight;
    uint32_t input;
    uint32_t output;
    float quant;
} CCNR_OP_MATQ;

typedef struct CCNR_OP_CONV2DQ {
    uint16_t op_type;
    uint16_t row;
    uint16_t col;
    uint8_t krow;
    uint8_t kcol;
    uint8_t nfilter;
    uint32_t weight;
    uint32_t input;
    uint32_t output;
    float quant;
} CCNR_OP_CONV2DQ;

typedef struct CCNR_OP_PCONV {
    uint16_t op_type;
    uint16_t row;
    uint16_t col;
    uint8_t nfilter;
    uint32_t weight;
    uint32_t input;
    uint32_t output;
    float quant;
} CCNR_OP_PCONV;

typedef struct CCNR_OP_ARR {
    uint16_t op_type;
    uint16_t row;
    uint32_t weight;
    uint32_t input;
    uint32_t output;
} CCNR_OP_ARR;

typedef struct CCNR_OP_ARRQ {
    uint16_t op_type;
    uint16_t row;
    uint32_t weight;
    uint32_t input;
    uint32_t output;
    float quant;
} CCNR_OP_ARRQ;

typedef struct CCNR_OP_CONS {
    uint16_t op_type;
    uint16_t row;
    uint32_t input;
    uint32_t output;
    float alpha;
} CCNR_OP_CONS;

typedef struct CCNR_OP_ACT {
    uint16_t op_type;
    uint16_t row;
    uint32_t input;
    uint32_t output;
} CCNR_OP_ACT;

typedef struct CCNR_OP_ACTQ {
    uint16_t op_type;
    uint16_t row;
    uint32_t input;
    uint32_t output;
    float quant;
} CCNR_OP_ACTQ;

#define INIT_OP_MAT(obj, op_type_, input_, output_, weight_, row_, col_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->col = col_; \
    obj->weight = weight_; \
    obj->input = input_;     \
    obj->output = output_;   \
    sizeof(*obj);})

#define INIT_OP_MATQ(obj, op_type_, input_, output_, weight_, row_, col_, quant_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->col = col_; \
    obj->weight = weight_; \
    obj->input = input_;     \
    obj->output = output_;   \
    obj->quant = quant_; \
    sizeof(*obj);})

#define INIT_OP_CONV2DQ(obj, op_type_, input_, output_, weight_, col_, row_, kcol_, krow_, nf_, quant_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->col = col_; \
    obj->krow = krow_; \
    obj->kcol = kcol_; \
    obj->nfilter = nf_; \
    obj->weight = weight_; \
    obj->input = input_;     \
    obj->output = output_;   \
    obj->quant = quant_; \
    sizeof(*obj);})

#define INIT_OP_PCONV(obj, op_type_, input_, output_, weight_, dlen_, convlen_, nf_, quant_) \
  ({obj->op_type = op_type_; \
    obj->row = dlen_; \
    obj->col = convlen_; \
    obj->nfilter = nf_; \
    obj->weight = weight_; \
    obj->input = input_;     \
    obj->output = output_;   \
    obj->quant = quant_; \
    sizeof(*obj);})

#define INIT_OP_ARR(obj, op_type_, input_, output_, weight_, row_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->weight = weight_; \
    obj->input = input_;     \
    obj->output = output_;   \
    sizeof(*obj);})

#define INIT_OP_ARRQ(obj, op_type_, input_, output_, weight_, row_, quant_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->weight = weight_; \
    obj->input = input_;     \
    obj->output = output_;   \
    obj->quant = quant_;     \
    sizeof(*obj);})

#define INIT_OP_CONS(obj, op_type_, alpha_, input_, output_, row_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->input = input_;     \
    obj->output = output_;   \
    obj->alpha = alpha_;     \
    sizeof(*obj);})

#define INIT_OP_ACT(obj, op_type_, input_, output_, row_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->input = input_;     \
    obj->output = output_;   \
    sizeof(*obj);})

#define INIT_OP_ACTQ(obj, op_type_, input_, output_, row_, quant_) \
  ({obj->op_type = op_type_; \
    obj->row = row_; \
    obj->input = input_;     \
    obj->output = output_;   \
    obj->quant = quant_;     \
    sizeof(*obj);})

#define MEMID(iaddr) (iaddr<<24)
#define ADDADDR(addr, offset) (addr + (offset))
#define ADDADDR2(addr, offset) (addr + ((offset) << 1))
#define ADDADDR4(addr, offset) (addr + ((offset) << 2))
#define ADDRIN 1
#define ADDROT 2
#define ADDRST 3
#define ADDRTM 4

int ccnn_execute(void **memtab, void* op, int opsize);

#endif
