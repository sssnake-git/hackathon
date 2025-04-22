import threading
import numpy as np
import random
import wave
import time
import sys
import re
import os

writeLog = False
targetSnr = [6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]

def loadfilelist(flist):
    files = []
    listfd = open(flist, 'r')
    parentdir = os.path.dirname(flist)
    while True:
        lines = listfd.readlines(100)
        if not lines:
            break
        for line in lines:
            larr = re.split('\s', line.strip())
            if len(larr) > 0:
                if os.path.exists(larr[0]):
                    files.append(larr[0])
                else:
                    fullpath = os.path.join(parentdir,larr[0])
                    if os.path.exists(fullpath):
                        files.append(fullpath)
                    else:
                        print("missing file", larr[0])

    listfd.close()
    return files

class NoiseMixer():
    def __init__(self, nth=4):
        self.con = threading.Condition()
        self.dlock = threading.Lock()
        self.wlock = threading.Lock()
        self.taskdata = None
        self.work = True
        self.threads = []
        for i in range(nth):
            self.threads.append(threading.Thread(target=self.processonetask))
            self.threads[-1].start()

    def gettaskdata(self):
        taskdata = None
        self.dlock.acquire()
        if self.work:
            self.con.acquire()
            while self.work:
                if self.taskdata != None:
                    taskdata = self.taskdata
                    self.taskdata = None
                    break
                else:
                    self.con.wait()
            self.con.notify()
            self.con.release()
        self.dlock.release()
        return taskdata

    def processonetask(self):
        while True:
            taskdata = self.gettaskdata()
            if taskdata is None:
                break
            voicefile, noisefile, mixSnr = taskdata
            mixout = voicefile
            mixout = mixout.replace("/ShareDisk01/WorkSpace/tianzhe/Asr/wav/fa", "/ShareDisk01/WorkSpace/tianzhe/Asr/wav/fa/696noi")
            # print(mixout)
            if not os.path.exists(os.path.dirname(mixout)):
                try:
                    os.makedirs(os.path.dirname(mixout))
                except OSError as exc: # Guard against race condition
                    if exc.errno != errno.EEXIST:
                        raise
            # mixout = self.outDir+os.path.basename(voicefile)
            # mixout = self.outDir+os.path.basename(voicefile).replace(".wav", "_"+os.path.basename(noisefile))

            with wave.open(voicefile, 'rb') as wavfile:
                fs_v = wavfile.getframerate()
                voclen = wavfile.getnframes()
                vocdata = np.frombuffer(wavfile.readframes(voclen), dtype=np.short).astype(np.float32)

            nsfd = open(noisefile, 'rb')
            with wave.open(nsfd, 'r') as wavfile:
                fs_n = wavfile.getframerate()
                nslen = wavfile.getnframes()
                nsbegin = wavfile.tell()
                nsoffset = 0
                if nslen > voclen:
                    nsoffset = np.random.randint(0, nslen-voclen)
                    nslen = voclen
                nsfd.seek(nsbegin+nsoffset*2, 0)
                nsdata = np.frombuffer(nsfd.read(nslen*2), dtype=np.short).astype(np.float32)
            nsfd.close()

            if fs_v != fs_n:
                print("voice samplerate not equal noise", voicefile, noisefile)
                continue

            if nslen < voclen:
                expandlen = nslen
                expanded = nsdata
                while expandlen < voclen - nslen:
                    expanded = np.concatenate((expanded, nsdata), 0)
                    expandlen += nslen
                if expandlen < voclen:
                    expanded = np.concatenate((expanded, nsdata[:voclen-expandlen]), 0)
                nsdata = expanded

            sum_s = np.sum(vocdata ** 2)
            sum_n = np.sum(nsdata ** 2)
            snw = np.sqrt(sum_s/(sum_n * pow(10, mixSnr/float(10)))).astype(np.float32)

            mixeddata = np.clip(vocdata + nsdata * snw, -32768, 32767)

            self.wlock.acquire()
            with wave.open(mixout, 'wb') as wavfile:
                wavfile.setparams((1, 2, fs_v, 0, 'NONE', 'NONE'))
                wavfile.writeframes(mixeddata.astype(np.int16))

            if self.mixlog != None:
                self.mixlog.write('{:s} {:s} {:s} {:.2f}\n'.format(voicefile, noisefile, mixout, mixSnr))
            self.wlock.release()

    def mix(self, argv):
        global writeLog
        fvoicelist = argv[0]
        fnslist = argv[1]
        outDir = argv[2]

        voicefiles = loadfilelist(fvoicelist)
        noisefiles = loadfilelist(fnslist)
        print("mix voice", len(voicefiles), " noise", len(noisefiles))
        if len(noisefiles) == 0 or len(voicefiles) == 0:
            return

        lt = time.localtime()
        timestr = time.strftime("%Y%m%d%H%M",lt)
        fmixlog = "noisemix_" + timestr + ".log"

        if outDir.endswith("/"):
            self.outDir = outDir
        else:
            self.outDir = outDir + "/"
        if not os.path.exists(self.outDir):
            os.mkdir(self.outDir)

        if writeLog:
            self.mixlog = open(fmixlog, 'w')
        else:
            self.mixlog = None

        np.random.shuffle(voicefiles)
        np.random.shuffle(noisefiles)

        nsindex = 0
        snrIdx = 0
        self.con.acquire()
        while len(voicefiles) > 0:
            voicefile = voicefiles.pop(0)
            noisefile = noisefiles[nsindex]
            snr = targetSnr[snrIdx]
            self.taskdata = (voicefile, noisefile, snr)
            self.con.notify()
            self.con.wait()

            nsindex += 1
            if nsindex >= len(noisefiles):
                nsindex = 0
            snrIdx += 1
            if snrIdx >= len(targetSnr):
                snrIdx = 0

        self.work = False
        self.con.notify()
        self.con.release()
        for th in self.threads:
            th.join()
        if self.mixlog != None:
            self.mixlog.close()

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print(sys.argv[0], "<wav lst> <noise wav list> outdir [-log]")
        exit(-1)
    if len(sys.argv) == 5 and sys.argv[-1] == "-log":
        writeLog = True
    mixer = NoiseMixer(5)
    mixer.mix(sys.argv[1:])
