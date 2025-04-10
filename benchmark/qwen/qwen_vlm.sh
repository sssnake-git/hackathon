#/bin/bash

prefix=~/code/llama.cpp/build/bin

${prefix}/llama-qwen2vl-cli -m ../../../weights/model/Qwen2.5-VL-3B-Instruct/merged_qwen2_vl_lora/mmproj-qwen2.5vl.gguf --mmproj ../../../weights/model/Qwen2-VL-7B-Instruct/qwen-qwen2-vl-7b-instruct-vision.gguf -p "请做一个检测任务, 你需要完成2个任务. 1 检测图片是否存在异常, 如果存在异常, 务必输出图像中异常的精确位置. 2 如果 不存在异常, 输出图片中不存在异常." --image ../../../cv/inside_detect/0408_test/z_10691.png
