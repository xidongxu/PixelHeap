#include <iostream>
#include <chrono>
#include <thread>
#include <SDL.h>
#include "audio.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#include "minimp3_ex.h"

using namespace std;

static size_t stream_read(void* buf, size_t size, void* user_data) {
    FILE* file = (FILE*)(user_data);
    if (file == nullptr)
        return 0;
    return fread(buf, 1, size, file);
}

static int stream_seek(uint64_t position, void* user_data) {
    FILE* file = (FILE*)(user_data);
    if (file == nullptr)
        return 0;
    return fseek(file, position, SEEK_SET);
}

static int frame_resample(uint8_t* input, int length, int src_rate, int dst_rate, uint8_t** output) {
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, 2, src_rate, AUDIO_S16SYS, 2, dst_rate) < 0)
        return -1;
    cvt.len = length;
    cvt.buf = (uint8_t*)malloc(cvt.len * cvt.len_mult);
    if (!cvt.buf)
        return -2;
    memcpy(cvt.buf, input, length);
    if (SDL_ConvertAudio(&cvt) < 0) {
        SDL_free(cvt.buf);
        return -3;
    }
    *output = cvt.buf;
    return cvt.len_cvt;
}

class AudioDecoder {
public:
    enum State { Pause, Exec, End, Quit };
	static AudioDecoder& instance() {
		static AudioDecoder m_instance{};
		return m_instance;
	}
	AudioDecoder(const AudioDecoder&) = delete;
	AudioDecoder& operator=(const AudioDecoder&) = delete;
	static void executor(AudioDecoder* decoder) {
        while (true) {
            if (decoder->m_state == Quit) {
                break;
            }
            if (decoder->m_state == Exec) {
                mp3dec_iterate_cb(&decoder->m_stream, decoder->m_buffer, MINIMP3_IO_SIZE, iterator, decoder);
            } else {
                std::this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
	}
    static int iterator(void* user_data, const uint8_t* frame, int frame_size, int free_format_bytes, size_t buf_size, uint64_t offset, mp3dec_frame_info_t* info) {
        (void)buf_size;
        (void)offset;
        (void)free_format_bytes;
        mp3d_sample_t *buffer = (mp3d_sample_t*)malloc(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t));
        if (buffer == nullptr)
            return -1;
        AudioDecoder* decoder = (AudioDecoder*)(user_data);
        int samples = mp3dec_decode_frame(&decoder->m_handle.mp3d, frame, frame_size, buffer, info) * sizeof(mp3d_sample_t);
        if (samples <= 0)
            return -2;
        uint8_t* data = nullptr;
        int size = frame_resample((uint8_t*)buffer, samples * info->channels, info->hz, 48000, &data);
        if (data == nullptr || size <= 0)
            return -3;
        if (decoder->m_player) {
            decoder->m_player->callback(decoder->m_player, data, size);
        }
        free(buffer);
        free(data);
        return 0;
    }
	int start(const string &url, uint64_t position, double *duration) {
        if (m_file != nullptr) {
            fclose(m_file);
        }
        m_file = fopen(url.c_str(), "rb");
        if (m_file == nullptr) {
            return -1;
        }
        m_stream.read_data = m_file;
        m_stream.read = stream_read;
        m_stream.seek_data = m_file;
        m_stream.seek = stream_seek;
        mp3dec_ex_open_cb(&m_handle, &m_stream, MP3D_SEEK_TO_BYTE);
        mp3dec_ex_seek(&m_handle, position);
        mp3d_sample_t* buffer{};
        mp3dec_frame_info_t frame{};
        size_t samples = mp3dec_ex_read_frame(&m_handle, &buffer, &frame, 1);
        *duration = (double)m_handle.samples / (frame.hz * frame.channels);
        m_state = (samples > 0) ? Exec : End;
		return (samples > 0) ? 0 : -2;
	}
	int puase() {
		return 0;
	}
	int resume() {
		return 0;
	}
	int stop() {
		return 0;
	}
    void setPlayer(AudioPlayer* player) {
        m_player = player;
    }

private:
	AudioDecoder() : m_state(Pause) {
		m_thread = std::thread([this] { AudioDecoder::executor(this); });
	}
	~AudioDecoder() {
		m_thread.join();
	}

private:
    State m_state{};
	std::thread m_thread{};
    FILE* m_file{};
	mp3dec_ex_t m_handle{};
	mp3dec_io_t m_stream{};
	uint8_t m_buffer[MINIMP3_IO_SIZE]{};
    AudioPlayer* m_player{};
};

class SDLPlayer : public AudioPlayer {
public:
    SDLPlayer() {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            std::cerr << "无法初始化 SDL: " << SDL_GetError() << std::endl;
            return;
        }
        SDL_AudioSpec audioSpec;
        audioSpec.freq = 48000;
        audioSpec.format = AUDIO_S16SYS;
        audioSpec.channels = 2;
        audioSpec.silence = 0;
        audioSpec.samples = 1024;
        audioSpec.callback = nullptr;
        // 打开音频设备
        if ((m_device = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0)) < 2) {
            cout << "open audio device failed " << endl;
        }
        SDL_PauseAudioDevice(m_device, 0);
    }
    ~SDLPlayer() {
        // 关闭音频设备
        if (m_device > 0) {
            SDL_PauseAudioDevice(m_device, 1);
            SDL_CloseAudioDevice(m_device);
        }
    }
    int callback(AudioPlayer* player, uint8_t* frame, int length) override {
        SDL_QueueAudio(m_device, frame, length);
        while (SDL_GetQueuedAudioSize(m_device) > 4096)
            SDL_Delay(10);
        return 0;
    }
    int play() override {
        m_decoder = &AudioDecoder::instance();
        if (m_decoder) {
            m_decoder->setPlayer(this);
            m_decoder->start(url(), 0, &m_duration);
            cout << "duration:" << m_duration << endl;
        }
        return 0;
    }
    int pause() override {
        return 0;
    }
    int stop() override {
        return 0;
    }
    int setPosition(double position) override {
        return 0;
    }
    double position() override {
        return m_position;
    }
    double duration() override {
        return m_duration;
    }

private:
    double m_position{};
    double m_duration{};
    AudioDecoder* m_decoder{};
    SDL_AudioDeviceID m_device{};
};

int main_audio(void) {
    AudioPlayer* player = new SDLPlayer();
    player->setUrl("assets/走过咖啡屋.mp3");
    player->play();
	return 0;
}
