// audio.h: 标准系统包含文件的包含文件
#pragma once
#include <String>

using namespace std;

class AudioPlayer {
public:
    enum Event {OnStop, OnPause, OnPlay, OnUpdate, OnEnded, OnError};
    virtual int play() = 0;
    virtual int pause() = 0;
    virtual int stop() = 0;
    virtual int setPosition(double position) = 0;
    virtual double position() = 0;
    virtual double duration() = 0;
    virtual int callback(Event event, uint8_t* frame = nullptr, int length = 0 , double position = 0.0) = 0;
    string& url() { return m_url; }
    void setUrl(const string& url) { m_url = url; }
    string& urlPlaying() { return m_urlPlaying; }
    void setUrlPlaying(const string& url) { m_urlPlaying = url; }

private:
    string m_url{};
    string m_urlPlaying{};
};

int main_audio(void);
