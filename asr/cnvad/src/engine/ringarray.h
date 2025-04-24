/**********************************************
 *  Author liudongqiang @ cerence             *
 **********************************************/

#ifndef __RINGARRAY_H__
#define __RINGARRAY_H__

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

typedef struct {
    int* buffer_;
    int front_;
    int back_;
    int size_;
    int cached_;
} RingArray;

void RingArray_Init(RingArray* rb, int size) {
    rb->buffer_ = NULL;
    rb->front_ = 0;
    rb->back_ = 0;
    rb->cached_ = 0;
    rb->size_ = 0;
    if (size > 0) {
        rb->buffer_ = (int*)malloc(size * sizeof(int));
        rb->size_ = size;
    }
}

void RingArray_Destroy(RingArray* rb) {
    if (rb->buffer_ != NULL) {
        free(rb->buffer_);
        rb->buffer_ = NULL;
    }
    rb->front_ = 0;
    rb->back_ = 0;
    rb->cached_ = 0;
    rb->size_ = 0;
}

int RingArray_Size(RingArray* rb) {
    return rb->cached_;
}

int RingArray_IsFull(RingArray* rb) {
    return rb->cached_ == rb->size_;
}

int RingArray_Write(RingArray* rb, const int value) {
    if (rb->cached_ < rb->size_) {
        rb->buffer_[rb->front_] = value;
        rb->front_++;
        rb->cached_++;
        if (rb->front_ == rb->size_) {
            rb->front_ = 0;
        }
        return 1;
    }
    return 0;
}

int RingArray_Max(RingArray* rb, int size) {
    int max = INT_MIN;
    int end = rb->back_ < rb->front_ ? rb->front_ : rb->size_;
    int skips = size > 0 ? rb->cached_ - size : 0;
    int i = rb->back_;
    if (skips > 0) {
        int skip = skips;
        if (skip > end - rb->back_) {
            skip = end - rb->back_;
        }
        i += skip;
        skips -= skip;
    }
    for (; i < end; i++) {
        if (rb->buffer_[i] > max) {
            max = rb->buffer_[i];
        }
    }
    if (rb->back_ >= rb->front_) {
        i = 0;
        if (skips > 0) {
            int skip = skips;
            if (skip > rb->front_) {
                skip = rb->front_;
            }
            i += skip;
        }
        for (; i < rb->front_; i++) {
            if (rb->buffer_[i] > max) {
                max = rb->buffer_[i];
            }
        }
    }
    return max;
}

int RingArray_Min(RingArray* rb, int min) {
    int end = rb->back_ < rb->front_ ? rb->front_ : rb->size_;
    for (int i = rb->back_; i < end; i++) {
        if (rb->buffer_[i] < min) {
            min = rb->buffer_[i];
        }
    }
    if (rb->back_ > rb->front_) {
        for (int i = 0; i < rb->front_; i++) {
            if (rb->buffer_[i] < min) {
                min = rb->buffer_[i];
            }
        }
    }
    return min;
}

int RingArray_Sum(RingArray* rb, int size) {
    int sum = 0;
    int end = rb->back_ < rb->front_ ? rb->front_ : rb->size_;
    int skips = size > 0 ? rb->cached_ - size : 0;
    int i = rb->back_;
    if (skips > 0) {
        int skip = skips;
        if (skip > end - rb->back_) {
            skip = end - rb->back_;
        }
        i += skip;
        skips -= skip;
    }
    for (; i < end; i++) {
        sum += rb->buffer_[i];
    }
    if (rb->back_ >= rb->front_) {
        i = 0;
        if (skips > 0) {
            int skip = skips;
            if (skip > rb->front_) {
                skip = rb->front_;
            }
            i += skip;
        }
        for (; i < rb->front_; i++) {
            sum += rb->buffer_[i];
        }
    }
    return sum;
}

void RingArray_Clear(RingArray* rb) {
    rb->front_ = 0;
    rb->back_ = 0;
    rb->cached_ = 0;
}

void RingArray_Drop(RingArray* rb, int size) {
    if (size >= rb->cached_) {
        rb->front_ = 0;
        rb->back_ = 0;
        rb->cached_ = 0;
    } else {
        rb->back_ = (rb->back_ + size) % rb->size_;
        rb->cached_ -= size;
    }
}

int RingArray_Get(RingArray* rb, int idx) {
    if (idx < 0) {
        idx = rb->cached_ + idx;
    }
    assert(idx >= 0 && idx < rb->cached_);
    idx = (rb->back_ + idx) % rb->size_;
    return rb->buffer_[idx];
}

#endif //__RINGARRAY_H__
