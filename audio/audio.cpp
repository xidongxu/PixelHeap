#include <iostream>
#include <chrono>
#include <thread>

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
        d->info->samples += samples * info->channels;
    }
    return 0;
}

int main_audio(void)
{
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
	return 0;
}
