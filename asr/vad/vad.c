#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "vad.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define true 1
#define false 0

static const char *tag = "[vad]";

static short write_ring_buffer(_ccrec_vad_ring_buf_ *_vad_buf, short *buffer,
                               short addLen);
static short read_ring_buffer(_ccrec_vad_ring_buf_ *_vad_buf, short *buffer,
                              short len);
static short reset_ring_buffer(_ccrec_vad_ring_buf_ *_vad_buf);

_ccrec_vad_ *vad_initialize(short nlength) {
    _ccrec_vad_ *_p_vad = (_ccrec_vad_ *)malloc(sizeof(_ccrec_vad_));

    if (!_p_vad) {
        return NULL;
    }
    memset(_p_vad, 0, sizeof(_ccrec_vad_));

    _p_vad->_p_buf =
        (_ccrec_vad_ring_buf_ *)malloc(sizeof(_ccrec_vad_ring_buf_));
    memset(_p_vad->_p_buf, 0, sizeof(_ccrec_vad_ring_buf_));

    if (!_p_vad->_p_buf) {
        free(_p_vad);
        return NULL;
    }

    _p_vad->buff_in_process = (short *)malloc(
        sizeof(short) * __CCREC_VAD_WINSIZE_SAMPLE__);

    if (!_p_vad->buff_in_process) {
        free(_p_vad->_p_buf);
        free(_p_vad);
        return NULL;
    }

    /* Ran: 12-27-2019: 返回buffer 的大小调整成功最大VAD_WINSIZE,
    因为 vad_initialize 函数不允许每次送给VAD 模块的数据超过VAD_WINSIZE,
    这样VAD模块不采用burst
    返回端点检测滞后数据的方式, 而是输入多少数据, 吐出多少数据
    */
    _p_vad->return_buffer = (short *)malloc(
        sizeof(short) * __CCREC_VAD_WINSIZE_SAMPLE__);
    memset(_p_vad->return_buffer, 0,
           (sizeof(short) * __CCREC_VAD_WINSIZE_SAMPLE__));

    if (!_p_vad->return_buffer) {
        free(_p_vad->buff_in_process);
        free(_p_vad->_p_buf);
        free(_p_vad);
        return NULL;
    }

    _p_vad->_p_buf->p_head =
        (short *)malloc(sizeof(short) * nlength);

    if (!_p_vad->_p_buf->p_head) {
        free(_p_vad->_p_buf);
        free(_p_vad);
        return NULL;
    }
    // memset(_p_vad->_p_buf->p_head,0,sizeof(short)*nlength);
    _p_vad->_p_buf->p_valid = _p_vad->_p_buf->p_valid_tail =
        _p_vad->_p_buf->p_head;
    _p_vad->_p_buf->p_tail = _p_vad->_p_buf->p_head + nlength;

    _p_vad->p_buf_offset =
        __CCREC_VAD_WINSIZE_SAMPLE__ - __CCREC_VAD_WINSHIFT_SAMPLE__;

    _p_vad->n_min_trailing_sil_smps = __CCREC_VAD_MIN_TS_SAMPLE__;
    _p_vad->f_abs_eng_th = __CCREC_VAD_ENG_ABSTH__;
    _p_vad->f_sensitivity = __CCREC_VAD_SENSITIVITY__;
    _p_vad->b_posssible_eos = false;
    _p_vad->f_avg_eng = 0;
    _p_vad->f_short_time_avg_eng = 0;
    return _p_vad;
}

/*
 *  * function:向缓冲区中写入数据
 *   * param:@buffer 写入的数据指针
 *    *       @addLen 写入的数据长度
 *     * return:-1:写入长度过大
 *      *        -2:缓冲区没有初始化
 */
static short write_ring_buffer(_ccrec_vad_ring_buf_ *_vad_buf, short *buffer,
                               short addLen) {
    if (addLen > __CCREC_VAD_RINGBUF_SIZE__) return -2;
    if (!_vad_buf || _vad_buf->p_head == NULL) return -1;

    // 将要存入的数据copy到pValidTail处
    if (_vad_buf->p_valid_tail + addLen > _vad_buf->p_tail) {
        // 需要分成两段copy
        int len1 = _vad_buf->p_tail - _vad_buf->p_valid_tail;
        int len2 = addLen - len1;
        memcpy(_vad_buf->p_valid_tail, buffer, sizeof(short) * len1);
        memcpy(_vad_buf->p_head, buffer + len1, sizeof(short) * len2);
        _vad_buf->p_valid_tail =
            _vad_buf->p_head + len2;  //新的有效数据区结尾指针
    } else {
        memcpy(_vad_buf->p_valid_tail, buffer, sizeof(short) * addLen);
        _vad_buf->p_valid_tail += addLen;  //新的有效数据区结尾指针
    }

    //需重新计算已使用区的起始位置
    if (_vad_buf->valid_len + addLen > __CCREC_VAD_RINGBUF_SIZE__) {
        int moveLen = _vad_buf->valid_len + addLen -
                      __CCREC_VAD_RINGBUF_SIZE__;  //有效指针将要移动的长度
        if (_vad_buf->p_valid + moveLen > _vad_buf->p_tail) {
            //需要分成两段计算
            int len1 = _vad_buf->p_tail - _vad_buf->p_valid;
            int len2 = moveLen - len1;
            _vad_buf->p_valid = _vad_buf->p_head + len2;
        } else {
            _vad_buf->p_valid = _vad_buf->p_valid + moveLen;
        }
        _vad_buf->valid_len = __CCREC_VAD_RINGBUF_SIZE__;
    } else {
        _vad_buf->valid_len += addLen;
    }

    return 0;
}
/*
 * function:从缓冲区内取出数据
 * param   :@buffer:接受读取数据的buffer
 *          @len:将要读取的数据的长度
 * return  :-1:没有初始化
 *          >0:实际读取的长度
 * */

static short read_ring_buffer(_ccrec_vad_ring_buf_ *_vad_buf, short *buffer,
                              short len) {
    if (!buffer || !_vad_buf || _vad_buf->p_head == NULL) return -1;

    if (len > __CCREC_VAD_RINGBUF_SIZE__)
        len = __CCREC_VAD_RINGBUF_SIZE__;  // 最多只能读取缓冲区最大长度

    if (_vad_buf->p_valid + len > _vad_buf->p_tail) {
        // 需要分成两段copy
        int len1 = _vad_buf->p_tail - _vad_buf->p_valid;
        int len2 = len - len1;
        memcpy(buffer, _vad_buf->p_valid, sizeof(short) * len1);  // 第一段
        memcpy(buffer + len1, _vad_buf->p_head,
               sizeof(short) * len2);  // 第二段, 绕到整个存储区的开头
        _vad_buf->p_valid = _vad_buf->p_head + len2;  // 更新已使用缓冲区的起始
    } else {
        memcpy(buffer, _vad_buf->p_valid, sizeof(short) * len);
        _vad_buf->p_valid = _vad_buf->p_valid + len;  // 更新已使用缓冲区的起始
    }
    _vad_buf->valid_len -= len;
    return len;
}

static short reset_ring_buffer(_ccrec_vad_ring_buf_ *_vad_buf) {
    if (!_vad_buf || _vad_buf->p_head == NULL) return -1;
    memset(_vad_buf->p_head, 0, sizeof(short) * __CCREC_VAD_RINGBUF_SIZE__);
    _vad_buf->valid_len = 0;
    _vad_buf->p_valid = _vad_buf->p_head;
    _vad_buf->p_valid_tail = _vad_buf->p_head + __CCREC_VAD_RINGBUF_SIZE__;

    return 0;
}

/* 往VAD 模块中送入数据: 
*  _p_inited_vad 是已经初始化过的vad 结构;
*  short* in_data 送入数据的指针头, short in_nSamples,  送入的数据大小
*  short** out_data 返回输出数据的指针头
*  isBOS_or_EOS_Buffer:  标记返回的out_data
是BOS的第一包数据, 还是EOS后应该送给解码器的最后一帧数据:  0:
这一包数据包含VAD的起点, 
   1: 这一包数据是解码器需要处理的最后一包数据, 处理完之后, 解码器应该重置开始下一次的解码, 
   -1: 其他
*  返回值: -1: 输入数据为空, 或者vad没有初始化
*          -2: 输入的数据长度太长, 超过一个处理窗的 buffer 大小;
           >0: 实际返回给后续模块可以使用的数据;

*/

short vad_add_wav_data(_ccrec_vad_ *_p_inited_vad, const short *in_data,
                       short in_nSamples, short **out_data,
                       short *isBOS_or_EOS_Buffer) {
    if ((!in_data && in_nSamples != 0) || !_p_inited_vad) {
        out_data = NULL;
        return -1;
    }

    if ((in_nSamples == 0 && in_data) || (in_nSamples > __CCREC_VAD_WINSIZE_SAMPLE__)) {
        out_data = NULL;
        return -2;
    }

    short ret = 0;
    short dataOffset = 0;
    short copySize = 0;
    short *_p_buf = _p_inited_vad->buff_in_process;

    while (dataOffset < in_nSamples) {
        copySize =
            min(in_nSamples - dataOffset,
                __CCREC_VAD_WINSIZE_SAMPLE__ - _p_inited_vad->p_buf_offset);
        memcpy(_p_buf + _p_inited_vad->p_buf_offset, in_data + dataOffset,
               sizeof(short) * copySize);
        _p_inited_vad->p_buf_offset += copySize;
        dataOffset += copySize;
        if (_p_inited_vad->p_buf_offset < __CCREC_VAD_WINSIZE_SAMPLE__) break;

        float fCurrEng = 0;
        fCurrEng = vad_calc_energy(_p_buf, __CCREC_VAD_WINSIZE_SAMPLE__);

        if (fCurrEng > _p_inited_vad->f_abs_eng_th + _p_inited_vad->f_bg_avg_eng) {
            _p_inited_vad->n_frame_count++;
            _p_inited_vad->f_acc_eng += fCurrEng;
            _p_inited_vad->f_avg_eng =
                _p_inited_vad->f_acc_eng / _p_inited_vad->n_frame_count;
        }
        if (!_p_inited_vad->b_bos_detected && !_p_inited_vad->b_possible_bos && (_p_inited_vad->f_bg_acc_eng + fCurrEng) / (_p_inited_vad->n_bg_idx + 1)  < _p_inited_vad->f_avg_eng) {
			_p_inited_vad->n_bg_idx++;
			_p_inited_vad->f_bg_acc_eng += fCurrEng;
			_p_inited_vad->f_bg_avg_eng = _p_inited_vad->f_bg_acc_eng/_p_inited_vad->n_bg_idx;
		}

        _p_inited_vad->n_idx++;

#if DEBUG
        printf("fr: %d\tAvgeng = %f\tSTAvgeng=%f\tcur_eng=%f\n",
                  _p_inited_vad->n_idx, _p_inited_vad->f_avg_eng,
                  _p_inited_vad->f_short_time_avg_eng, fCurrEng);
#endif

        if (!_p_inited_vad->b_bos_detected) {
            // if (!_p_inited_vad->b_possible_bos &&
            //     fCurrEng >
            //         _p_inited_vad->f_sensitivity * _p_inited_vad->f_avg_eng + _p_inited_vad->f_bg_avg_eng && _p_inited_vad->f_avg_eng > 0) {
            // Xuran Changed on Mar 29 2023.
            if (!_p_inited_vad->b_possible_bos &&
                fCurrEng >
                    _p_inited_vad->f_sensitivity * _p_inited_vad->f_avg_eng && _p_inited_vad->f_avg_eng > 0) {
                // 第一帧满足严格能量阈值的突变点检测到, 可能是一个Bos, 往后继续观察
                _p_inited_vad->b_possible_bos = true;
                // _p_inited_vad->n_ms_det_speech +=
                // __CCREC_VAD_WINSHIFT_SAMPLE__;
                _p_inited_vad->f_short_time_acc_eng =
                    _p_inited_vad->f_short_time_avg_eng =
                        (fCurrEng + _p_inited_vad->f_avg_eng) / 2.0f;
                _p_inited_vad->n_short_time_frame_count = 1;
#if DEBUG
                printf("#possbile BOS...%d, STAvgeng = %f, Avgeng = %f, \
                          cur_eng = %f\n",
                          _p_inited_vad->n_idx,
                          _p_inited_vad->f_short_time_avg_eng,
                          _p_inited_vad->f_avg_eng, fCurrEng);
#endif

            // } else if (_p_inited_vad->b_possible_bos &&
            //            (fCurrEng > _p_inited_vad->f_short_time_avg_eng + _p_inited_vad->f_bg_avg_eng ||
            //             fCurrEng > _p_inited_vad->f_sensitivity *
            //                            _p_inited_vad->f_avg_eng + _p_inited_vad->f_bg_avg_eng)) {
            // Xuran Changed on Mar 29 2023.
            } else if (_p_inited_vad->b_possible_bos &&
                (fCurrEng > _p_inited_vad->f_short_time_avg_eng ||
                fCurrEng > _p_inited_vad->f_sensitivity * _p_inited_vad->f_avg_eng)) {
                // possible Bos
                // 之后的能量只要满足于短时能量判断, 则继续累计DetSpeech
                _p_inited_vad->n_ms_det_speech += __CCREC_VAD_WINSHIFT_SAMPLE__;
                _p_inited_vad->f_short_time_acc_eng += fCurrEng;
                _p_inited_vad->n_short_time_frame_count++;
                _p_inited_vad->f_short_time_avg_eng =
                    _p_inited_vad->f_short_time_acc_eng /
                    _p_inited_vad->n_short_time_frame_count;
            // } else if (_p_inited_vad->b_possible_bos &&
            //            fCurrEng < _p_inited_vad->f_short_time_avg_eng + _p_inited_vad->f_bg_avg_eng) {
            // Xuran Changed on Mar 29 2023.
            } else if (_p_inited_vad->b_possible_bos &&
                       fCurrEng < _p_inited_vad->f_short_time_avg_eng) {
                _p_inited_vad->b_possible_bos = false;
                _p_inited_vad->n_ms_det_speech = 0;
                _p_inited_vad->f_short_time_avg_eng = 0;
                _p_inited_vad->f_short_time_acc_eng = 0;
                _p_inited_vad->n_short_time_frame_count = 0;
            } else if ( fCurrEng >
                    _p_inited_vad->f_sensitivity * _p_inited_vad->f_avg_eng) {
                //reset bg
                if (_p_inited_vad->f_avg_eng > _p_inited_vad->f_bg_avg_eng && fCurrEng > _p_inited_vad->f_bg_avg_eng) {
                    _p_inited_vad->b_possible_bos = true;
                    _p_inited_vad->f_short_time_acc_eng =
                        _p_inited_vad->f_short_time_avg_eng =
                            (fCurrEng + _p_inited_vad->f_avg_eng) / 2.0f;
                    _p_inited_vad->n_short_time_frame_count = 1;
                }
                _p_inited_vad->n_bg_idx = 0;
                _p_inited_vad->f_bg_acc_eng = 0;
                _p_inited_vad->f_bg_avg_eng = 0;
            }

            // if (_p_inited_vad->n_ms_det_speech >=
            //         __CCREC_VAD_MIN_SPEECH_SAMPLE__ &&
            //     _p_inited_vad->f_avg_eng > _p_inited_vad->f_abs_eng_th + _p_inited_vad->f_bg_avg_eng) {
            // Xuran Changed on Mar 29 2023.
            if (_p_inited_vad->n_ms_det_speech >=
                    __CCREC_VAD_MIN_SPEECH_SAMPLE__ &&
                _p_inited_vad->f_avg_eng > _p_inited_vad->f_abs_eng_th) {
                /* BOS detetected: 连续检测到的语音满足最小语音长度门限*/
                /*
                ret = read_ring_buffer(_p_inited_vad->_p_buf,
                _p_inited_vad->return_buffer,
                    min(_p_inited_vad->_p_buf->valid_len, in_nSamples));
                write_ring_buffer(_p_inited_vad->_p_buf, _p_buf,
                __CCREC_VAD_WINSIZE_SAMPLE__); if(dataOffset < in_nSamples){
                    write_ring_buffer(_p_inited_vad->_p_buf, in_data+dataOffset,
                (in_nSamples - dataOffset));
                }
                */
#if DEBUG
                log_info(
                    tag, "#BOS detected at: %d, Avgeng = %f, cur_eng=%f\n",
                    _p_inited_vad->n_idx - 1 - _p_inited_vad->n_ms_det_speech / 160, _p_inited_vad->f_avg_eng, fCurrEng);
#endif

                printf("#BOS detected at: %d ms.\n", 
                        (_p_inited_vad->n_idx - 1 - _p_inited_vad->n_ms_det_speech / 160) * 10);

                _p_inited_vad->n_ms_det_speech = 0;
                // 平均能量和累计能量改为当前帧的能量
                // _p_inited_vad->f_acc_eng = fCurrEng;
                // _p_inited_vad->f_avg_eng = fCurrEng;
                // _p_inited_vad->n_frame_count = 1;

                _p_inited_vad->f_short_time_avg_eng = 0;
                _p_inited_vad->f_short_time_acc_eng = 0;
                _p_inited_vad->n_short_time_frame_count = 0;

                _p_inited_vad->b_bos_detected = true;
                _p_inited_vad->b_possible_bos = false;
                _p_inited_vad->b_posssible_eos = false;
                _p_inited_vad->b_eos_detected = false;
                *isBOS_or_EOS_Buffer =
                    0;  // 标记此返回buffer是BOS检测到的第一包数据, 
                // *out_data = _p_inited_vad->return_buffer;
                // return ret;
            }
        } else {
            // Adding EOS logic here
            // if (!_p_inited_vad->b_posssible_eos &&
            //     fCurrEng <
            //         __CCREC_VAD_EOS_SENSITIVITY__ * _p_inited_vad->f_avg_eng + _p_inited_vad->f_bg_avg_eng) {
            // Xuran Changed on Mar 29 2023.
            if (!_p_inited_vad->b_posssible_eos &&
                fCurrEng <
                    __CCREC_VAD_EOS_SENSITIVITY__ * _p_inited_vad->f_avg_eng) {
                _p_inited_vad->b_posssible_eos = true;
                // _p_inited_vad->n_ms_det_trailing_sil +=
                // __CCREC_VAD_WINSHIFT_SAMPLE__;
                _p_inited_vad->f_short_time_acc_eng =
                    _p_inited_vad->f_short_time_avg_eng =
                        _p_inited_vad->f_avg_eng;
                _p_inited_vad->n_short_time_frame_count = 1;

#if DEBUG
                printf("#possbile EOS...%d, STAvgeng=%f, Avgeng = %f, \
                          cur_eng = % f\n ",
                          _p_inited_vad->n_idx,
                          _p_inited_vad->f_short_time_avg_eng,
                          _p_inited_vad->f_avg_eng, fCurrEng);
#endif

            // } else if (_p_inited_vad->b_posssible_eos &&
            //            fCurrEng < max(_p_inited_vad->f_avg_eng,
            //                           _p_inited_vad->f_short_time_avg_eng) + _p_inited_vad->f_bg_avg_eng) {
            // Xuran Changed on Mar 29 2023.
            } else if (_p_inited_vad->b_posssible_eos &&
                       fCurrEng < max(_p_inited_vad->f_avg_eng,
                                      _p_inited_vad->f_short_time_avg_eng)) {
                // possible Eos
                // 之后的能量只要满足于短时能量判断, 则继续累计DetSpeech
                _p_inited_vad->n_ms_det_trailing_sil +=
                    __CCREC_VAD_WINSHIFT_SAMPLE__;
                _p_inited_vad->f_short_time_acc_eng += fCurrEng;
                _p_inited_vad->n_short_time_frame_count++;
                _p_inited_vad->f_short_time_avg_eng =
                    _p_inited_vad->f_short_time_acc_eng /
                    _p_inited_vad->n_short_time_frame_count;
            // } else if (_p_inited_vad->b_posssible_eos &&
            //            fCurrEng > _p_inited_vad->f_avg_eng &&
            //            _p_inited_vad->f_avg_eng > _p_inited_vad->f_abs_eng_th + _p_inited_vad->f_bg_avg_eng) {
            // Xuran Changed on Mar 29 2023.
            } else if (_p_inited_vad->b_posssible_eos &&
                       fCurrEng > _p_inited_vad->f_avg_eng &&
                       _p_inited_vad->f_avg_eng > _p_inited_vad->f_abs_eng_th) {
                // 不是一个可能的尾点
                _p_inited_vad->b_posssible_eos = false;
                _p_inited_vad->n_ms_det_trailing_sil = 0;
                _p_inited_vad->f_short_time_avg_eng = 0;
                _p_inited_vad->f_short_time_acc_eng = 0;
                _p_inited_vad->n_short_time_frame_count = 0;
            }
            if (_p_inited_vad->n_ms_det_trailing_sil >=
                _p_inited_vad->n_min_trailing_sil_smps) {
                /* EOS detetected: 连续检测到的静音满足最小TS长度门限
                    计算延后关闸的采样点个数*/

                _p_inited_vad->n_delayed_writing_smps =
                    __CCREC_VAD_WINSHIFT_SAMPLE__ +
                    _p_inited_vad->_p_buf->valid_len;

                _p_inited_vad->n_ms_det_trailing_sil = 0;
                // 平均能量和累计能量改为当前帧的能量
                _p_inited_vad->f_acc_eng = 0;
                _p_inited_vad->f_avg_eng = 0;
                _p_inited_vad->f_short_time_avg_eng = 0;
                _p_inited_vad->f_short_time_acc_eng = 0;
                _p_inited_vad->n_short_time_frame_count = 0;
                _p_inited_vad->n_frame_count = 1;
                _p_inited_vad->b_bos_detected = false;
                _p_inited_vad->b_possible_bos = false;
                _p_inited_vad->b_posssible_eos = false;
                _p_inited_vad->b_eos_detected = true;

                //reset bg
                _p_inited_vad->n_bg_idx = 0;
                _p_inited_vad->f_bg_acc_eng = 0;
                _p_inited_vad->f_bg_avg_eng = 0;

#if DEBUG
                log_info(
                    tag, "#EOS detected at: %d, Avgeng = %f, cur_eng=%f\n",
                    _p_inited_vad->n_idx, _p_inited_vad->f_avg_eng, fCurrEng);
#endif

                printf("#EOS detected at: %d ms.\n", _p_inited_vad->n_idx * 10);
            }
        }

        _p_inited_vad->p_buf_offset -= __CCREC_VAD_WINSHIFT_SAMPLE__;
        ret = write_ring_buffer(_p_inited_vad->_p_buf, _p_buf,
                                __CCREC_VAD_WINSHIFT_SAMPLE__);
        *isBOS_or_EOS_Buffer = -1;  // 标记buffer没有任何特殊标记, 
        if (ret < 0) {
            printf("rec=%d, write buff error!\n", ret);
            return ret;
        }
        memmove(_p_buf, _p_buf + __CCREC_VAD_WINSHIFT_SAMPLE__,
                sizeof(short) * (_p_inited_vad->p_buf_offset));
    }

    // 如果是已经检测到起点的状态, 直接从缓冲区往外吐数据, 
    if (_p_inited_vad->b_bos_detected) {
        // 如果在上一个EOS
        // 检测到之后, 延迟需要吐的数据还没有拿完的时候再次检测到起点, 则继续把之前没有送完的数据送完, 
        if (_p_inited_vad->n_delayed_writing_smps > 0) {
            int writedata = min(_p_inited_vad->_p_buf->valid_len, in_nSamples);
            ret = read_ring_buffer(
                _p_inited_vad->_p_buf, _p_inited_vad->return_buffer,
                min(writedata, _p_inited_vad->n_delayed_writing_smps));
            _p_inited_vad->n_delayed_writing_smps -= ret;
            *out_data = _p_inited_vad->return_buffer;
            if (_p_inited_vad->n_delayed_writing_smps <= 0) {
                *isBOS_or_EOS_Buffer = 1;  // 标记此返回buffer是最后一包数据, 
            } else {
                *isBOS_or_EOS_Buffer = -1;  // 改回来之前被BOS标记过的符号
            }
            return ret;
        }
        ret = read_ring_buffer(
            _p_inited_vad->_p_buf, _p_inited_vad->return_buffer,
            min(_p_inited_vad->_p_buf->valid_len, in_nSamples));
        *out_data = _p_inited_vad->return_buffer;
        return ret;
    } else if (_p_inited_vad->b_eos_detected &&
               _p_inited_vad->n_delayed_writing_smps > 0) {
        // 如果是尾点检测到的状态, 则需要继续往外送数据直到延迟TS时间达到
        // Original: after eos detected, send 256 data each time util reaches
        // trailing silence Now: after eos detected, send all data once keep
        // original code for backup, it is commented
#if defined(__CCREC_VAD_CUT_TS__)
        int writedata = min(_p_inited_vad->_p_buf->valid_len, __CCREC_VAD_WINSIZE_SAMPLE__);
#else
        int writedata = min(_p_inited_vad->_p_buf->valid_len, in_nSamples);
#endif
        ret = read_ring_buffer(
            _p_inited_vad->_p_buf, _p_inited_vad->return_buffer,
            min(writedata, _p_inited_vad->n_delayed_writing_smps));
        _p_inited_vad->n_delayed_writing_smps -= ret;
        *out_data = _p_inited_vad->return_buffer;
        if (_p_inited_vad->n_delayed_writing_smps <= 0) {
            *isBOS_or_EOS_Buffer = 1;  // 标记此返回buffer是最后一包数据
        } else {
#if defined(__CCREC_VAD_CUT_TS__)
            *isBOS_or_EOS_Buffer = 2; // Need to read vad buffer data without new data coming
#else
            // do nothing
#endif
        }

        return ret;
    } else {
        out_data = NULL;
        return 0;
    }
}

float vad_calc_energy(short *_p_buf, short data_len) {
    if (data_len <= 0) return 0;

    float fe = 0;
    int i;
    for (i = 0; i < data_len; i++) {
        if (_p_buf[i] > 0) {
            fe += _p_buf[i];
        } else {
            fe -= _p_buf[i];
        }
    }
    fe /= data_len;

    return fe;
}

void vad_reset(_ccrec_vad_ *_p_inited_vad) {
    if (!_p_inited_vad) return;
    _p_inited_vad->b_bos_detected = false; /* true if a start point found */
    _p_inited_vad->b_possible_bos =
        false; /* true if the first frame matching > sensitivity*favgEng */
    _p_inited_vad->b_eos_detected = false;
    _p_inited_vad->b_posssible_eos = false;

    _p_inited_vad->f_acc_eng = 0;
    _p_inited_vad->f_avg_eng = 0; /* average energy from the beginning of the
                                     signal to current frame */
    _p_inited_vad->f_short_time_acc_eng = 0;
    _p_inited_vad->f_short_time_avg_eng =
        0; /* short-time avg energy between possile_bos and realbos */

    //_p_inited_vad->p_buf_offset = __CCREC_VAD_WINSIZE_SAMPLE__ -
    //__CCREC_VAD_WINSHIFT_SAMPLE__;
    _p_inited_vad->n_frame_count =
        0; /* accumulated frames until current processing before VAD detected */
    _p_inited_vad->n_short_time_frame_count =
        0; /* short-time frames for short-time avg energy calculation */

    printf("reset buffer at %d\n", _p_inited_vad->n_idx);
    //_p_inited_vad->n_idx = 0; /* frame ID only for debug
    // usage*/

    _p_inited_vad->n_ms_det_speech =
        0; /* detected speech before determing it is real VAD point */
    _p_inited_vad->n_ms_det_trailing_sil =
        0; /* detected trailing silence before a real EOS deteded */
    _p_inited_vad->n_delayed_writing_smps = 0;

    //reset bg
    _p_inited_vad->n_bg_idx = 0;
    _p_inited_vad->f_bg_acc_eng = 0;
    _p_inited_vad->f_bg_avg_eng = 0;

    // memset(_p_inited_vad->buff_in_process, 0,
    // sizeof(short)*__CCREC_VAD_WINSIZE_SAMPLE__);
    // memset(_p_inited_vad->return_buffer, 0,
    // sizeof(short)*__CCREC_VAD_WINSIZE_SAMPLE__);

    // if(!_p_inited_vad->_p_buf) return;
    // reset_ring_buffer(_p_inited_vad->_p_buf);
}

void vad_release(_ccrec_vad_ *_p_vad) {
    if (_p_vad) {
        if (_p_vad->_p_buf) {
            if (_p_vad->_p_buf->p_head) {
                free(_p_vad->_p_buf->p_head);
            }
            free(_p_vad->_p_buf);
        }
        if (_p_vad->buff_in_process) free(_p_vad->buff_in_process);
        if (_p_vad->return_buffer) free(_p_vad->return_buffer);
        free(_p_vad);
    }
}

void vad_set_min_ts_smps(_ccrec_vad_ *_p_inited_vad, unsigned int val) {
    _p_inited_vad->n_min_trailing_sil_smps = val;
}

void vad_set_abs_eng_th(_ccrec_vad_ *_p_inited_vad, float val) {
    _p_inited_vad->f_abs_eng_th = val;
}

void vad_set_sensitivity(_ccrec_vad_ *_p_inited_vad, float val) {
    _p_inited_vad->f_sensitivity = val;
}
