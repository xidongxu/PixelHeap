#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
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
		free(cvt.buf);
		return -3;
	}
	*output = cvt.buf;
	return cvt.len_cvt;
}

class Semaphore {
public:
	explicit Semaphore(int count = 0) : m_count(count) {}

	void notify() {
		std::unique_lock<std::mutex> lock(m_mutex);
		++m_count;
		m_cond.notify_one();
	}

	void wait() {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cond.wait(lock, [this]() { return m_count > 0; });
		--m_count;
	}

	bool wait_for(int time) {
		std::unique_lock<std::mutex> lock(m_mutex);
		auto result = m_cond.wait_for(lock, std::chrono::milliseconds(time), [this]() { return m_count > 0; });
		if (result) {
			--m_count;
		}
		return result;
	}
private:
	int m_count;
	std::mutex m_mutex;
	std::condition_variable m_cond;
};

template <typename StateType>
class StateUtil {
public:
	explicit StateUtil(StateType state = StateType())
		: m_origin(state), m_target(state) {
	}
	StateType operator()() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_origin;
	}
	StateType target() {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_target;
	}
	void notify(StateType state) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_target == state && m_origin != state) {
			m_origin = state;
			m_signal.notify();
		}
	}
	void wait(StateType state) {
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_target = state;
			while (m_signal.wait_for(0)) {}
		}
		m_signal.wait();
	}
	void reset(StateType state) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_origin = m_target = state;
		m_signal.notify();
	}
private:
	std::mutex m_mutex{};
	StateType m_origin{};
	StateType m_target{};
	Semaphore m_signal{};
};

typedef struct mp3dec_reader {
	mp3dec_ex_t mp3dec;
	mp3dec_io_t stream;
	size_t buf_size;
	size_t filled;
	size_t consumed;
	int eof;
	int index;
	int frame_size;
	uint64_t readed;
	uint8_t buffer[MINIMP3_IO_SIZE]{};
} mp3dec_reader_t;

int mp3dec_reader_init(mp3dec_reader_t *reader, uint64_t position, uint64_t *offset) {
	if (!reader || (size_t)-1 == sizeof(reader->buffer) || sizeof(reader->buffer) < MINIMP3_BUF_SIZE) {
		return MP3D_E_PARAM;
	}
	int result = 0;
	mp3dec_io_t* io = &reader->stream;
	reader->buf_size = sizeof(reader->buffer);
	result = mp3dec_ex_open_cb(&reader->mp3dec, io, MP3D_SEEK_TO_BYTE);
	if (result < 0) {
		return result;
	}
	position = position * (reader->mp3dec.info.bitrate_kbps * 1000 / 8);
	position = position + reader->mp3dec.start_offset;
	*offset = position;
	result = mp3dec_ex_seek(&reader->mp3dec, position);
	if (result < 0) {
		return result;
	}
	reader->filled = io->read(reader->buffer, MINIMP3_ID3_DETECT_SIZE, io->read_data);
	reader->consumed = 0;
	reader->readed = 0;
	reader->eof = 0;
	if (reader->filled > MINIMP3_ID3_DETECT_SIZE) {
		return MP3D_E_IOERROR;
	}
	if (MINIMP3_ID3_DETECT_SIZE != reader->filled) {
		return 0;
	}
	size_t id3v2size = mp3dec_skip_id3v2(reader->buffer, reader->filled);
	if (id3v2size) {
		if (io->seek(id3v2size, io->seek_data)) {
			return MP3D_E_IOERROR;
		}
		reader->filled = io->read(reader->buffer, reader->buf_size, io->read_data);
		if (reader->filled > reader->buf_size) {
			return MP3D_E_IOERROR;
		}
		reader->readed += id3v2size;
	} else {
		size_t readed = io->read(reader->buffer + MINIMP3_ID3_DETECT_SIZE, reader->buf_size - MINIMP3_ID3_DETECT_SIZE, io->read_data);
		if (readed > (reader->buf_size - MINIMP3_ID3_DETECT_SIZE)) {
			return MP3D_E_IOERROR;
		}
		reader->filled += readed;
	}
	if (reader->filled < MINIMP3_BUF_SIZE) {
		mp3dec_skip_id3v1(reader->buffer, &reader->filled);
	}
	return 0;
}

int mp3dec_reader_deinit(mp3dec_reader_t* reader) {
	mp3dec_ex_close(&reader->mp3dec);
	return 0;
}

int mp3dec_reader_read(mp3dec_reader_t* reader, const uint8_t **frame, mp3dec_frame_info_t *frame_info) {
	if (!reader || !reader->buffer || (size_t)-1 == reader->buf_size || reader->buf_size < MINIMP3_BUF_SIZE) {
		return MP3D_E_PARAM;
	}
	while (true) {
		mp3dec_io_t* io = &reader->stream;
		reader->readed += reader->frame_size;
		reader->consumed += reader->index + reader->frame_size;
		if (!reader->eof && reader->filled - reader->consumed < MINIMP3_BUF_SIZE) {
			/* keep minimum 10 consecutive mp3 frames (~16KB) worst case */
			std::memmove(reader->buffer, reader->buffer + reader->consumed, reader->filled - reader->consumed);
			reader->filled -= reader->consumed;
			reader->consumed = 0;
			size_t readed = io->read(reader->buffer + reader->filled, reader->buf_size - reader->filled, io->read_data);
			if (readed > (reader->buf_size - reader->filled)) {
				return MP3D_E_IOERROR;
			}
			if (readed != (reader->buf_size - reader->filled)) {
				reader->eof = 1;
			}
			reader->filled += readed;
			if (reader->eof) {
				mp3dec_skip_id3v1(reader->buffer, &reader->filled);
			}
		}
		int free_bytes = 0;
		reader->index = mp3d_find_frame(reader->buffer + reader->consumed, reader->filled - reader->consumed, &free_bytes, &reader->frame_size);
		if (reader->index && !reader->frame_size) {
			reader->consumed += reader->index;
			continue;
		}
		if (!reader->frame_size) {
			break;
		}
		const uint8_t* hdr = reader->buffer + reader->consumed + reader->index;
		if (frame_info != nullptr) {
			frame_info->channels = HDR_IS_MONO(hdr) ? 1 : 2;
			frame_info->hz = hdr_sample_rate_hz(hdr);
			frame_info->layer = 4 - HDR_GET_LAYER(hdr);
			frame_info->bitrate_kbps = hdr_bitrate_kbps(hdr);
			frame_info->frame_bytes = reader->frame_size;
		}
		reader->readed += reader->index;
		*frame = hdr;
		return reader->frame_size;
	}
	return 0;
}

class AudioDecoder {
public:
	enum class State { Stop, Pause, Exec, Quit };
	AudioDecoder(AudioPlayer *player) : m_player(player) {
		m_thread = std::thread([this] { AudioDecoder::executor(this); });
	}
	~AudioDecoder() {
		m_thread.join();
	}
	static void executor(AudioDecoder* decoder) {
		while (true) {
			if (decoder->state.target() == State::Pause) {
				decoder->state.notify(State::Pause);
				std::this_thread::sleep_for(chrono::milliseconds(100));
			}
			if (decoder->state.target() == State::Stop) {
				decoder->state.notify(State::Stop);
				std::this_thread::sleep_for(chrono::milliseconds(100));
			}
			if (decoder->state.target() == State::Exec) {
				decoder->state.notify(State::Exec);
				const uint8_t* frame = nullptr;
				mp3dec_frame_info_t info{};
				int frame_size = mp3dec_reader_read(&decoder->m_reader, &frame, &info);
				if (frame_size <= 0) {
					decoder->state.reset(State::Stop);
					decoder->m_player->callback(AudioPlayer::OnEnded);
					continue;
				}
				decoder->m_offset += frame_size;
				double position = double(decoder->m_offset) / (double(info.bitrate_kbps) * 1000.0 / 8.0);
				mp3d_sample_t* buffer = (mp3d_sample_t*)malloc(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t));
				if (buffer == nullptr) {
					continue;
				}
				int samples = mp3dec_decode_frame(&decoder->m_reader.mp3dec.mp3d, frame, frame_size, buffer, &info) * sizeof(mp3d_sample_t);
				if (samples < 0) {
					continue;
				}
				uint8_t* data = nullptr;
				int size = frame_resample((uint8_t*)buffer, samples * info.channels, info.hz, 48000, &data);
				if (data == nullptr || size <= 0) {
					continue;
				}
				decoder->m_player->callback(AudioPlayer::OnUpdate, data, size, position);
				free(buffer);
				free(data);
			}
			if (decoder->state.target() == State::Quit) {
				break;
			}
		}
		decoder->state.notify(State::Quit);
	}
	int start(const string& url, uint64_t position, double* duration) {
		int result = -1;
		if (state() == State::Quit) {
			return result;
		}
		if (state() != State::Stop) {
			stop();
		}
		m_file = fopen(url.c_str(), "rb");
		if (m_file == nullptr) {
			return result;
		}
		m_reader.stream.read_data = m_file;
		m_reader.stream.read = stream_read;
		m_reader.stream.seek_data = m_file;
		m_reader.stream.seek = stream_seek;
		result = mp3dec_reader_init(&m_reader, position, &m_offset);
		if (result == 0) {
			*duration = (double)m_reader.mp3dec.samples / (m_reader.mp3dec.info.hz * m_reader.mp3dec.info.channels);
			state.wait(State::Exec);
		}
		m_seek = (m_reader.mp3dec.vbr_tag_found == 0);
		return result;
	}
	int puase() {
		if (state() == State::Exec) {
			state.wait(State::Pause);
			m_player->callback(AudioPlayer::OnPause);
			return 0;
		}
		return -1;
	}
	int resume() {
		if (state() == State::Pause) {
			state.wait(State::Exec);
			m_player->callback(AudioPlayer::OnPlay);
			return 0;
		}
		return -1;
	}
	int stop() {
		if (state() == State::Quit || state() == State::Stop) {
			return -1;
		}
		state.wait(State::Stop);
		if (m_file != nullptr) {
			fclose(m_file);
		}
		mp3dec_reader_deinit(&m_reader);
		m_player->callback(AudioPlayer::OnStop);
		return 0;
	}
	int seekAble() {
		return m_seek;
	}
public:
	StateUtil<State> state{};

private:
	bool m_seek{};
	FILE* m_file{};
	uint64_t m_offset{};
	std::thread m_thread{};
	mp3dec_reader_t m_reader{};
	AudioPlayer* m_player{};
};

class SDLPlayer : public AudioPlayer {
public:
	enum Status { Stop, Pause, Play, Ended, Error };
	SDLPlayer() : m_decoder(this) {
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
	int callback(Event event, uint8_t* frame, int length, double position) override {
		switch (event) {
		case OnPlay:
			m_status = Play;
			break;
		case OnPause:
			m_status = Pause;
			break;
		case OnStop:
			m_status = Stop;
			break;
		case OnEnded:
			m_status = Ended;
			break;
		case OnError:
			m_status = Error;
			break;
		case OnUpdate:
			m_position = position;
			cout << " position = " << m_position << "/" << m_duration << endl;
			SDL_QueueAudio(m_device, frame, length);
			while (SDL_GetQueuedAudioSize(m_device) > 4096) {
				SDL_Delay(10);
			}
			break;
		default:
			cout << "player recive error event:" << event;
			break;
		}
		return 0;
	}
	int play() override {
		if (url() == urlPlaying()) {
			m_decoder.resume();
			return 0;
		}
		int result = m_decoder.start(url(), 0, &m_duration);
		if (result >= 0) {
			setUrlPlaying(url());
			cout << "duration:" << m_duration << endl;
		} else {
			cout << "url " << url() << " playing failed:" << result << endl;
		}
		return 0;
	}
	int pause() override {
		m_decoder.puase();
		return 0;
	}
	int stop() override {
		m_decoder.stop();
		return 0;
	}
	int setPosition(double position) override {
		m_decoder.stop();
		m_decoder.start(url(), position, &m_duration);
		return 0;
	}
	double position() override {
		return m_position;
	}
	double duration() override {
		return m_duration;
	}

private:
	Status m_status{};
	double m_position{};
	double m_duration{};
	AudioDecoder m_decoder;
	SDL_AudioDeviceID m_device{};
};

int main_audio(void) {
	AudioPlayer* player = new SDLPlayer();
	player->setUrl("assets/走过咖啡屋.mp3");
	player->play();
	auto m_thread = std::thread([player] {
		std::this_thread::sleep_for(chrono::seconds(5));
		cout << "================== pause ==================" << endl;
		player->pause();
		std::this_thread::sleep_for(chrono::seconds(5));
		cout << "================== start ==================" << endl;
		player->play();
		std::this_thread::sleep_for(chrono::seconds(5));
		cout << "================== set position  ==================" << endl;
		player->setPosition(30);
		std::this_thread::sleep_for(chrono::seconds(5));
		cout << "================== set position  ==================" << endl;
		player->setPosition(60);
		std::this_thread::sleep_for(chrono::seconds(5));
		cout << "================== set position  ==================" << endl;
		player->setPosition(90);
		std::this_thread::sleep_for(chrono::seconds(10000));
	});
	m_thread.join();
	return 0;
}
