#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "denoise.h"

/*
test in.pcm out.pcm
test in.wav out.wav
sox in.wav -t raw -c 1 -e signed-integer -b 16 -r 16000 - | test -t raw - out.pcm
sox in.wav -t raw -c 1 -e signed-integer -b 16 -r 16000 - | test -t raw - -t raw - | sox -t raw -c 1 -e signed-integer -b 16 -r 16000 out.wav
*/

int main(int argc, char **argv) {
#if 0
    FILE *file_output;
    FILE *file_input;

    if (argc != 3) {
        printf("%s <input> <output>.\n", argv[0]);
        return -1;
    }

    char *input_file_name = argv[1];
    char *output_file_name = argv[2];

    if (((file_input = fopen(input_file_name, "rb" )) == NULL )) {
        printf( "The input file could not be opened\n" );
        exit (1);
    }

    if (((file_output = fopen(output_file_name, "wb")) == NULL)) {
        printf("The output file could not be opened\n");
        exit(1);
    }

    short *in = (short *)malloc(sizeof(short) * NN);
#else
    int c, ioptf = 0;
    int inwav = -1;
    int outwav = -1;
    char *fn_in = 0, *fn_out = 0;
    while (optind < argc) {
        if (strlen(argv[optind]) > 1 && argv[optind][0] == '-') {
            if ((c = getopt(argc, argv, "t:")) >= 0)
            {
                if (c == 't') {
                    int iswav = strcmp(optarg, "wav")?0:1;
                    if (ioptf == 0) {
                        inwav = iswav;
                    } else if (ioptf == 1) {
                        outwav = iswav;
                    } else {
                        fprintf(stderr, "%s params error\n", argv[0]);
                        return -1;
                    }
                }
            } else {
                fprintf(stderr, "%s params error\n", argv[0]);
                return -1;
            }
        } else {
            if (ioptf == 0) {
                fn_in = argv[optind];
            } else if (ioptf == 1) {
                fn_out = argv[optind];
            }
            ioptf++;
            optind++;
        }
    }

    if (!fn_in) {
        fprintf(stderr, "%s params error\n", argv[0]);
        return -1;
    }

    if (!fn_in) {
        fprintf(stderr, "%s params error\n", argv[0]);
        return -1;
    }

    FILE *file_input = strcmp(fn_in, "-") ? fopen(fn_in, "rb") : stdin;
    if (NULL == file_input) {
        fprintf(stderr, "error open %s\n", fn_in);
        return -1;
    }

    char *finddot, *dot;
    dot = finddot = strchr(fn_in, '.');
    while (finddot) {
        dot = finddot;
        finddot = strchr(finddot+1, '.');
    }
    
    short *in = (short *)malloc(sizeof(short) * NN);

    if ((dot != NULL && !strncmp(dot+1, "wav", 3)) && inwav) {
        fread(in, 1, 44, file_input);
        inwav = 1;
    } else if (inwav == 1) {
        fread(in, 1, 44, file_input);
    }

    if (outwav == -1) {
        dot = finddot = strchr(fn_out, '.');
        while (finddot) {
            dot = finddot;
            finddot = strchr(finddot+1, '.');
        }
        if (dot && !strncmp(dot+1, "wav", 3))
            outwav = 1;
    }

    FILE* file_output = NULL;
    if (fn_out) {
        file_output = strcmp(fn_out, "-") ? fopen(fn_out, "wb") : stdout;
    }

    if (inwav==1 && outwav==1 && file_output) {
        fwrite(in, 44, 1, file_output);
    }
#endif

    // sub = (specsub*)calloc(1, sizeof(specsub));
    // sub = (specsub*)malloc(sizeof(specsub));
    // sub = &g_sub;
    // spec_sub_ini(sub);

    specsub *specsub_hdl;
    specsub_hdl = (specsub *)malloc(sizeof(specsub));
    memset(specsub_hdl, 0, sizeof(specsub));
    spec_sub_ini(specsub_hdl);
    int readlen = 0;

#if 0
    // handle wav head
    short *wav_head = (short *)malloc(sizeof(short) * 22);
    if (strstr(input_file_name, "wav") != NULL) {
        printf("Remove wav head.\n");
        if (fread(wav_head, sizeof(short), 22, file_input) != 22) {
            printf("Error reading wav header from %s!\n", input_file_name);
            return -1;
        }

        // fwrite(wav_head, sizeof(short), 22, file_output);
    }
#endif

    while ((readlen = fread(in, sizeof(short), NN, file_input)) == NN) {
        if (feof(file_input))
            break;

        spec_sub(specsub_hdl, in);
        fwrite(in, sizeof(short), NN, file_output);
#if 1
        readlen = 0;
#endif
    }
#if 1
    if (readlen != 0) {
        fwrite(in, sizeof(short), readlen, file_output);
    }
#endif
    // speex_preprocess_state_destroy(st);
    // spec_sub_deini(sub);
    fclose(file_input);
    fclose(file_output);

    spec_sub_deini(specsub_hdl);
    free(in);
    // free(wav_head);
    // return 0;
}
