#include "stdio.h"
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <memory.h>
#include <sys/time.h>
#include "project_config.h"
#include "cnvad.h"

#define BOOL char
#define TRUE 1
#define FALSE 0
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

const char* helpstr = "[-t [wav/raw]] audiofile [tagoutfile]";

int main(int argc, char* argv[]) {

    int framelen = FRAMESHIFT;
    short data[framelen];

    int c, ioptf = 0;
    int inwav = -1;
    char *fn_in = 0, *fn_out = 0;
    BOOL simulat = FALSE;

    while (optind < argc) {
        if (strlen(argv[optind]) > 1 && argv[optind][0] == '-') {
            if ((c = getopt(argc, argv, "t:s")) >= 0) {
                if (c == 't') {
                    int iswav = strcmp(optarg, "wav")?0:1;
                    if (ioptf == 0) {
                        inwav = iswav;
                    } else {
                        fprintf(stderr, "%s %s\n", argv[0], helpstr);
                        return -1;
                    }
                } else if (c == 's') {
                    simulat = TRUE;
                }
            } else {
                fprintf(stderr, "%s %s\n", argv[0], helpstr);
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
        fprintf(stderr, "%s %s\n", argv[0], helpstr);
        return -1;
    }

    FILE *fp = strcmp(fn_in, "-") ? fopen(fn_in, "rb") : stdin;
    if (NULL == fp) {
        fprintf(stderr, "error open %s\n", fn_in);
        return -1;
    }

    char *finddot, *dot;
    dot = finddot = strchr(fn_in, '.');
    while (finddot) {
        dot = finddot;
        finddot = strchr(finddot+1, '.');
    }

    if ((dot != NULL && !strncmp(dot + 1, "wav", 3)) && inwav) {
        // fseek(fp, 44, SEEK_SET);
        fread(data, 1, 44, fp);
        inwav = 1;
    } else if (inwav == 1) {
        fread(data, 1, 44, fp);
    }

    FILE* fp_out = NULL;
    if (fn_out) {
        fp_out = strcmp(fn_out, "-") ? fopen(fn_out, "wb") : stdout;
    }

    void* cnvadinst = vad_initialize(NULL);

    if (!cnvadinst) {
        fprintf(stderr, "cnvad init error\n");
        return -1;
    }

    // char *segpath = "/mnt/hgfs/E/Media/Audio/wavsplit/";
    char *segpath = NULL;
    FILE *segoutfp = NULL;

    char inspeech = 0;
    int nprocessed = 0;
    int outsize;
    int vadcount = 0;
    float vadtime[2];
    char printbuf[64];

    struct timeval ts, tn;
    gettimeofday(&ts, NULL);
    while (fread(data, sizeof(short), framelen, fp) >= framelen) {
        if (simulat) {
            int64_t datatime = nprocessed * framelen * 1000 / 16000;
            gettimeofday(&tn, NULL);
            int64_t runtime = (tn.tv_sec - ts.tv_sec) * 1000 + (tn.tv_usec - ts.tv_usec) / 1000;
            if (runtime < datatime) {
                usleep((datatime - runtime) * 1000);
            }
        }

        short *outdata;
        unsigned int bos = 0, eos = 0;
        short isBOS_or_EOS_Buffer;
        outsize = vad_add_wav_data(cnvadinst, data, framelen, &outdata, &bos, &eos, &isBOS_or_EOS_Buffer);
        if (bos) {
            vadtime[0] = max(bos*framelen/16, 0)/1000.0;
            if (!fp_out) {
                printf("%.3f %d\n", vadtime[0], 1);
            }
            vadcount++;
            inspeech = 1;
        }

        if (segoutfp && outsize) {
            fwrite(outdata, sizeof(short), outsize, segoutfp);
        }

        if (eos) {
            vadtime[1] = (eos*framelen/16)/1000.0;
            if (fp_out) {
                int slen = snprintf(printbuf, 32, "%.2f\t%.2f\n", vadtime[0], vadtime[1]);
                fwrite(printbuf, slen, 1, fp_out);
            } else {
                printf("%.3f %d\n", vadtime[1], 0);
            }

            while (isBOS_or_EOS_Buffer != 1) {
                outsize = vad_add_wav_data(cnvadinst, NULL, framelen, &outdata, &bos, &eos, &isBOS_or_EOS_Buffer);
                if (segoutfp && outsize > 0) {
                    fwrite(outdata, sizeof(short), outsize, segoutfp);
                }
            }
            if (segoutfp) {
                fclose(segoutfp);
                segoutfp = NULL;
            }
            inspeech = 0;
        }

        if (isBOS_or_EOS_Buffer == 0 && segpath) {
            snprintf(printbuf, 64, "%svad_%d.pcm", segpath, vadcount);
            printf("write %s\n", printbuf);
            segoutfp = fopen(printbuf, "wb");
            if (segoutfp && outsize > 0) {
                fwrite(outdata, sizeof(short), outsize, segoutfp);
            }
        }

        nprocessed++;
    }

    if (fp_out && inspeech) {
        vadtime[1] = (nprocessed*framelen/16)/1000.0;
        int slen = snprintf(printbuf, 32, "%.3f\t%.3f\n", vadtime[0], vadtime[1]);
        fwrite(printbuf, slen, 1, fp_out);
    }

    gettimeofday(&tn, NULL);
    int runtime = (tn.tv_sec - ts.tv_sec)*1000 + (tn.tv_usec - ts.tv_usec)/1000;
    fprintf(stderr, "file len %.3fs processed %dms vad %d\n", nprocessed*framelen/16000.0f, runtime, vadcount);

    if (fp_out) {
        fflush(fp_out);
        if (fp_out != stdout)
            fclose(fp_out);
    }
    if (fp) {
        fclose(fp);
    }
    vad_release(cnvadinst);

    return 0;
}
