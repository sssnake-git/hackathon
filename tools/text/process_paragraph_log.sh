#!/bin/bash

# 处理以段落为格式的log

if [ $# != 2 ]; then
    echo "Usage: $0 <log> <keyword>"
    exit 1
fi

log=$1
keyword=$2

awk -v RS= -v kw="$keyword" '{
    if (NR > 1) {
        # 在上一个段落中查找含有 begin, asr 和 vad 字段的行并打印
        if (asr !~ /"$asr"/ && asr !~ kw) {
            print "-----------------------"
            print begin
            print asr
            print vad
        }
    }
    # 保存当前段落的内容
    paragraph = $0

    # 在当前段落中查找含有 begin, asr 和 vad 字段的行
    begin = ""
    asr = ""
    vad = ""
    lines = split(paragraph, a, "\n")
    for (i = 1; i <= lines; i++) {
        if (a[i] ~ /begin/) {
            begin = begin a[i] "\n"
        }
        if (a[i] ~ /asr/) {
            asr = substr(a[i], index(a[i], ",")+1)
        }
        if (a[i] ~ /vad/) {
            vad = vad a[i] "\n"
        }
    }
}
END {
    # 打印最后一个段落中含有 begin, asr 和 vad 字段的行
    if (asr !~ /"$asr"/ && asr !~ kw) {
        print "-----------------------"
        print begin
        print asr
        print vad
    }
}' < $log
