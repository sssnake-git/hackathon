/*
 * This file is part of pocketfft.
 * Licensed under a 3-clause BSD style license - see LICENSE.md
 */

/*! \file pocketfft.h
 *  Public interface of the pocketfft library
 *
 *  Copyright (C) 2008-2018 Max-Planck-Society
 *  \author Martin Reinecke
 */

#ifndef __POCKETFFT_H__
#define __POCKETFFT_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
struct cfft_plan_i;
typedef struct cfft_plan_i * cfft_plan;
cfft_plan make_cfft_plan (size_t length);
void destroy_cfft_plan (cfft_plan plan);
int cfft_backward(cfft_plan plan, float c[], float fct);
int cfft_forward(cfft_plan plan, float c[], float fct);
size_t cfft_length(cfft_plan plan);

struct rfft_plan_i;
typedef struct rfft_plan_i * rfft_plan;
rfft_plan make_rfft_plan (size_t length);
void destroy_rfft_plan (rfft_plan plan);
int rfft_backward(rfft_plan plan, float c[], float fct);
int rfft_forward(rfft_plan plan, float c[], float fct);
size_t rfft_length(rfft_plan plan);
#endif

#define NFCT 25
typedef struct rfftp_fctdata
{
    size_t fct;
    float *tw, *tws;
} rfftp_fctdata;
typedef struct rfftp_plan_i
{
    size_t length, nfct;
    float *mem;
    rfftp_fctdata fct[NFCT];
} rfftp_plan_i;

typedef struct rfftp_plan_i * rfftp_plan;

rfftp_plan make_rfftp_plan (size_t length);
int rfftp_forward(rfftp_plan plan, float c[], float fct);
int rfftp_backward(rfftp_plan plan, float c[], float fct);
void destroy_rfftp_plan (rfftp_plan plan);

#ifdef __cplusplus
}
#endif

#endif
