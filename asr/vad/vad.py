import os
import sys
import wave
import webrtcvad

def read_wave(path):
    """读取音频文件"""
    with wave.open(path, 'rb') as wf:
        num_channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        sample_rate = wf.getframerate()
        frames = wf.readframes(wf.getnframes())
        return frames, sample_rate, num_channels, sample_width

def write_wave(path, audio, sample_rate, sample_width):
    """写入音频文件"""
    with wave.open(path, 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(sample_width)
        wf.setframerate(sample_rate)
        wf.writeframes(audio)

def vad_segment(input_file, output_dir):
    """根据VAD切割音频文件"""
    frames, sample_rate, num_channels, sample_width = read_wave(input_file)
    vad = webrtcvad.Vad()
    vad.set_mode(3)  # 设置VAD模式, 0-3, 越高越激进
    window_duration = 30  # 每一帧的长度(ms)
    window_size = int(window_duration * sample_rate / 1000)  # 每一帧的长度(采样点)
    num_samples = len(frames) // sample_width  # 音频采样点数
    num_frames = num_samples // window_size  # 音频帧数
    segment_start = 0  # 当前分段的起始位置
    audio_chunks = []  # 切割后的音频片段
    speech = False  # 标记是否在语音段内
    for i in range(num_frames):
        start = i * window_size
        end = start + window_size
        frame = frames[start * sample_width:end * sample_width]
        is_speech = vad.is_speech(frame, sample_rate)
        if is_speech and not speech:  # 进入语音段
            segment_start = start
            speech = True
        elif not is_speech and speech:  # 离开语音段
            segment_end = start
            audio_chunks.append(frames[segment_start * sample_width:segment_end * sample_width])
            speech = False
    if speech:  # 最后一段语音
        audio_chunks.append(frames[segment_start * sample_width:])

    # 将音频片段保存为文件
    for i, chunk in enumerate(audio_chunks):
        output_file = os.path.join(output_dir, f"chunk{i}.wav")
        write_wave(output_file, chunk, sample_rate, sample_width)
    
if __name__ == "__main__":
    vad_segment('/mnt/d/Asr/wav/emb/eng/indiafr/Aarti/Change_Light_Color.wav', './')

