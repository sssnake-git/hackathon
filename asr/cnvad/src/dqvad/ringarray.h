/**********************************************
 *  Author liudongqiang @ cerence             *
 **********************************************/

#ifndef __RING_ARRAY_H__
#define __RING_ARRAY_H__

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
#if 0
int RingArray_Capacity(RingArray* rb) {
    return rb->size_;
}
#endif
int RingArray_IsFull(RingArray* rb) {
    return rb->cached_ == rb->size_;
}
#if 0
void RingArray_SetAll(RingArray* rb, const int value) {
    if (rb->size_ > 0) {
        for (int i = 0; i < rb->size_; i++) {
            rb->buffer_[i] = value;
        }
        rb->front_ = 0;
        rb->back_ = 0;
        rb->cached_ = rb->size_;
    }
}
#endif
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
#if 0
int RingArray_WriteData(RingArray* rb, const int* data, int size) {
    const int* data_pos = data;
    while (rb->cached_ < rb->size_ && size > 0) {
        int copysize = size;
        if (rb->front_ >= rb->back_) {
            if (rb->front_ + size > rb->size_) {
                copysize = rb->size_ - rb->front_;
            }
            memcpy(rb->buffer_ + rb->front_, data_pos, copysize * sizeof(int));
        } else {
            int leftsize = rb->back_ - rb->front_;
            if (copysize > leftsize) {
                copysize = leftsize;
            }
            memcpy(rb->buffer_ + rb->front_, data_pos, copysize * sizeof(int));
        }
        rb->front_ += copysize;
        data_pos += copysize;
        rb->cached_ += copysize;
        size -= copysize;
        if (rb->front_ == rb->size_) {
            rb->front_ = 0;
        }
    }
    return (int)(data_pos - data);
}

int RingArray_Read(RingArray* rb, int* data, int size) {
    int* data_pos = data;
    while (rb->cached_ > 0 && size > 0) {
        int readsize = size;
        if (rb->back_ < rb->front_) {
            if (rb->cached_ < readsize) {
                readsize = rb->front_ - rb->back_;
            }
            memcpy(data_pos, rb->buffer_ + rb->back_, readsize * sizeof(int));
        } else {
            int leftsize = rb->size_ - rb->back_;
            if (readsize > leftsize) {
                readsize = leftsize;
            }
            memcpy(data_pos, rb->buffer_ + rb->back_, readsize * sizeof(int));
        }
        rb->back_ += readsize;
        data_pos += readsize;
        rb->cached_ -= readsize;
        size -= readsize;
        if (rb->back_ == rb->size_) {
            rb->back_ = 0;
        }
    }
    return (int)(data_pos - data);
}
#endif
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

#endif //__RING_ARRAY_H__
