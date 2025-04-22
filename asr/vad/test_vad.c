#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vad.h"

#define __CCREC_TEST_VAD_MEM_POOL_SIZE__ (4096 * 1024)

static unsigned int ccrec_vad_heap[__CCREC_TEST_VAD_MEM_POOL_SIZE__ / sizeof(unsigned int)];

#define __CCREC_TEST_VAD_INPUT_BUF_SIZE__ 256

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("%s <input audio>.\n", argv[0]);
        return -1;
    }
    char *audio_fn = argv[1];
    FILE *fp = fopen(audio_fn, "rb");

    static _ccrec_vad_ *_p_vad;
    _p_vad = vad_initialize(4096);
    if (!_p_vad) {
        printf("Vad initialization failed!\n");
        return -1;
    }

    int16_t *data = (int16_t *)malloc(sizeof(int16_t) * __CCREC_TEST_VAD_INPUT_BUF_SIZE__);
    memset(data, 0, sizeof(int16_t) * __CCREC_TEST_VAD_INPUT_BUF_SIZE__);
    int file_size = ftell(fp);
    rewind(fp);

    int deal_count = 0;
    int prev_ret = 0;
    bool valid_utt = false;
    int utt_index = 0;

    if (strcasecmp(audio_fn, ".wav") == 0) {
        if (fread(data, sizeof(int16_t), 22, fp) != 22) {
            printf("Error reading wav header from %s!\n", audio_fn);
            return -1;
        }
        file_size -= sizeof(int16_t) * 22;
    }

    char *res_utt_name = (char *)malloc(sizeof(char) * 64);
    memset(res_utt_name, 0, sizeof(char) * 64);
    sprintf(res_utt_name, "%d.pcm", utt_index);
    FILE *res_utt_fp = fopen(res_utt_name, "w");

    // read 256 dots each time
    while (fread(data, sizeof(int16_t), __CCREC_TEST_VAD_INPUT_BUF_SIZE__, fp) == __CCREC_TEST_VAD_INPUT_BUF_SIZE__) {
        file_size -= sizeof(int16_t) * __CCREC_TEST_VAD_INPUT_BUF_SIZE__;

        int16_t *vad_data = NULL;
        short is_eos_or_bos = 0;
        int ret = vad_add_wav_data(_p_vad, data, __CCREC_TEST_VAD_INPUT_BUF_SIZE__, &vad_data, &is_eos_or_bos);
        // printf("is_eos_or_bos: %d, ret: %d\n", is_eos_or_bos, ret);

        if (ret != prev_ret) {
            if (ret == __CCREC_TEST_VAD_INPUT_BUF_SIZE__) {
                valid_utt = true;
            } else {
                valid_utt = false;
                // data_eos = deal_count;
            }
        }
        prev_ret = ret;

        // if (data_eos == 0 && valid_utt && file_size < sizeof(__int16_t) * VAD_INPUT_BUF_SIZE) {
        //     data_eos = deal_count;
        // }

        if (valid_utt) {
            fwrite(vad_data, sizeof(__int16_t), ret, res_utt_fp);
            if (is_eos_or_bos >= 0) {
                fclose(res_utt_fp);
                utt_index += 1;
                memset(res_utt_name, 0, sizeof(char) * 64);
                sprintf(res_utt_name, "%d.pcm", utt_index);
                res_utt_fp = fopen(res_utt_name, "w");
            }
        }
    }
    free(data);
    free(res_utt_name);
    vad_release(_p_vad);
    fclose(fp);
    if (res_utt_fp) {
        fclose(res_utt_fp);
    }

    return 0;
}