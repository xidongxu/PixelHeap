#include <iostream>
#include <chrono>
#include <thread>
#include <SDL.h>
#include "audio.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#include "minimp3_ex.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

using namespace std;

static size_t streamRead(void* buf, size_t size, void* user_data) {
    return fread(buf, 1, size, (FILE*)user_data);
}

static int streamSeek(uint64_t position, void* user_data) {
    return fseek((FILE*)user_data, position, SEEK_SET);
}

typedef struct {
    mp3dec_t* mp3d;
    mp3dec_file_info_t* info;
    SDL_AudioDeviceID deviceId;
} frames_iterate_data;

int resample_audio(uint8_t* input, int length, int src_rate, int dst_rate, uint8_t** output) {
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, 2, src_rate, AUDIO_S16SYS, 2, dst_rate) < 0)
        return -1;
    cvt.len = length;
    cvt.buf = (uint8_t *)malloc(cvt.len * cvt.len_mult);
    if (!cvt.buf)
        return - 2;
    memcpy(cvt.buf, input, length);
    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_free(cvt.buf);
        return - 3;
    }
    *output = cvt.buf;
    return cvt.len_cvt;
}

static int frames_iterate_cb(void* user_data, const uint8_t* frame, int frame_size, int free_format_bytes, size_t buf_size, uint64_t offset, mp3dec_frame_info_t* info) {
    (void)buf_size;
    (void)offset;
    (void)free_format_bytes;
    frames_iterate_data* user = (frames_iterate_data *)user_data;
    user->info->channels = info->channels;
    user->info->hz = info->hz;
    user->info->layer = info->layer;
    user->info->buffer = (mp3d_sample_t*)malloc(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t));
    if (user->info->buffer == nullptr)
        return -1;
    int samples = mp3dec_decode_frame(user->mp3d, frame, frame_size, user->info->buffer, info) * sizeof(mp3d_sample_t);
    if (samples <= 0)
        return -2;
    uint8_t* data = nullptr;
    int size = resample_audio((uint8_t*)user->info->buffer, samples * info->channels, info->hz, 48000, &data);
    if (data == nullptr || size <= 0)
        return -3;
    SDL_QueueAudio(user->deviceId, data, size);
    free(user->info->buffer);
    free(data);
    while (SDL_GetQueuedAudioSize(user->deviceId) > 4096)
        SDL_Delay(10);
    return 0;
}

int main_audio(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "无法初始化 SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_AudioDeviceID deviceId;
    SDL_AudioSpec audioSpec;
    audioSpec.freq = 48000;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 2;
    audioSpec.silence = 0;
    audioSpec.samples = 1024;
    audioSpec.callback = nullptr;
    // 打开音频设备
    if ((deviceId = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0)) < 2) {
        cout << "open audio device failed " << endl;
        return -1;
    }
    SDL_PauseAudioDevice(deviceId, 0);
    // 打开解码线程
    thread m_thread = std::thread([deviceId] {
        mp3dec_t mp3d;
        mp3dec_io_t io;
        mp3dec_file_info_t info;
        memset(&info, 0, sizeof(info));
        io.read = streamRead;
        io.seek = streamSeek;
        uint8_t* io_buf = (uint8_t*)malloc(MINIMP3_IO_SIZE);
        FILE* file = fopen("assets/走过咖啡屋.mp3", "rb");
        io.read_data = io.seek_data = file;
        frames_iterate_data user = { &mp3d, &info, deviceId };
        mp3dec_init(&mp3d);
        mp3dec_iterate_cb(&io, io_buf, MINIMP3_IO_SIZE, frames_iterate_cb, &user);
        free(io_buf);
    });
    m_thread.join();
	return 0;
}
