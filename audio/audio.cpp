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

static size_t streamRead(void* buf, size_t size, void* user_data)
{
    return fread(buf, 1, size, (FILE*)user_data);
}

static int streamSeek(uint64_t position, void* user_data)
{
    return fseek((FILE*)user_data, position, SEEK_SET);
}

typedef struct
{
    mp3dec_t* mp3d;
    mp3dec_file_info_t* info;
    size_t allocated;
} frames_iterate_data;

SDL_AudioDeviceID deviceId;

static int frames_iterate_cb(void* user_data, const uint8_t* frame, int frame_size, int free_format_bytes, size_t buf_size, uint64_t offset, mp3dec_frame_info_t* info)
{
    (void)buf_size;
    (void)offset;
    (void)free_format_bytes;
    frames_iterate_data* d = (frames_iterate_data *)user_data;
    d->info->channels = info->channels;
    d->info->hz = info->hz;
    d->info->layer = info->layer;
    printf("%d %d %d\n", frame_size, (int)offset, info->channels);
    if ((d->allocated - d->info->samples * sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t))
    {
        if (!d->allocated)
            d->allocated = 1024 * 1024;
        else
            d->allocated *= 2;
        mp3d_sample_t* alloc_buf = (mp3d_sample_t*)realloc(d->info->buffer, d->allocated);
        if (!alloc_buf)
            return MP3D_E_MEMORY;
        d->info->buffer = alloc_buf;
    }
    int samples = mp3dec_decode_frame(d->mp3d, frame, frame_size, d->info->buffer + d->info->samples, info);
    if (samples)
    {
        SDL_QueueAudio(deviceId, d->info->buffer + d->info->samples, samples);
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        d->info->samples += samples * info->channels;
    }
    return 0;
}

int main_audio(void)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "无法初始化 SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_AudioSpec audioSpec;
    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 2;
    audioSpec.silence = 0;
    audioSpec.samples = 1024;
    audioSpec.callback = nullptr; // 因为是推模式，所以这里为 nullptr
    // 打开音频设备
    if ((deviceId = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2) {
        cout << "open audio device failed " << endl;
        return -1;
    }
    SDL_PauseAudioDevice(deviceId, 0);

    thread m_thread = std::thread([] {
        mp3dec_t mp3d;
        mp3dec_io_t io;
        mp3dec_file_info_t info;
        memset(&info, 0, sizeof(info));
        io.read = streamRead;
        io.seek = streamSeek;
        uint8_t* io_buf = (uint8_t*)malloc(MINIMP3_IO_SIZE);
        FILE* file = fopen("assets/走过咖啡屋.mp3", "rb");
        io.read_data = io.seek_data = file;
        frames_iterate_data d = { &mp3d, &info, 0 };
        mp3dec_init(&mp3d);
        mp3dec_iterate_cb(&io, io_buf, MINIMP3_IO_SIZE, frames_iterate_cb, &d);
    });
    m_thread.join();
	return 0;
}
