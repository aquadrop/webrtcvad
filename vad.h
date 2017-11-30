#ifndef WEBRTC_VAD_H_
#define WEBRTC_VAD_H_

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/typedefs.h"
#include <deque>

// vad wrapper for webrtc vad

class Vad {
public:
    // @param mode, set its aggressiveness mode, which is an integer 
    //              between 0 and 3. 0 is the least aggressive about 
    //              filtering out non-speech, 3 is the most aggressive
    Vad(int mode, unsigned short num_channels, unsigned long sample_rate,
        unsigned short bits_per_sample, unsigned int window_duration_ms, unsigned int frame_duration_ms); 
    virtual ~Vad();
    void SetMode(int mode);
    bool IsSpeech(const int16_t *data, int num_point_per_frame, int sample_rate);
    char SlideWindow(int tag);
    float _VOICE_THRED_ = 0.9f;
private:
    int mode_;
    VadInst* handle_;
    bool trigger_;
    std::deque<int> window_;
    int window_len_ = 0;
    int window_sum_ = 0;

    unsigned short num_channels;
    unsigned long sample_rate;
    unsigned short bits_per_sample;
    unsigned int window_duration_ms; // ms
    unsigned int frame_duration_ms; // ms

    unsigned short block_align;
    unsigned int bytes_per_sec;
    unsigned int bytes_per_unit;
    int num_point_per_frame;
    int num_data;
    int num_sample;

    int buff_size;
};


#endif
