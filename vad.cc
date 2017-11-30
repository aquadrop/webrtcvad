#include "vad.h"
#include "stdio.h"
#include "stdlib.h"

Vad::Vad(int mode): mode_(mode), trigger_(false), window_len_(10), window_sum_(0) {
    if (WebRtcVad_Create(&handle_) < 0) {
        printf("Create webrtc vad handle error\n");
        exit(-1);
    }
    WebRtcVad_Init(handle_);
    WebRtcVad_set_mode(handle_, mode_);
}

Vad::~Vad() {
    WebRtcVad_Free(handle_);
}

void Vad::SetMode(int mode) {
    mode_ = mode;
    WebRtcVad_set_mode(handle_, mode_);
}

bool Vad::IsSpeech(const int16_t *data, int num_point_per_frame, int sample_rate) {
    int ret = WebRtcVad_Process(handle_, sample_rate, data, num_point_per_frame);
    switch (ret) {
        case 0:
            return false;
        case 1:
            return true;
        default:
            printf("WebRtcVad_Process error, check sample rate or frame length\n");
            exit(-1);
            return false;
    }
}

char Vad::GetFrameState(const int16_t *data, int frame_len, int sample_rate) {
    int num_point_per_frame = (int)(frame_len * sample_rate / 1000);
    bool tag = IsSpeech(data, num_point_per_frame, sample_rate);
    if (window_.size() < window_len_) {
        window_sum_ += tag;
        window_.push_back(tag);
        return 'N';
    } else {
        window_sum_ += tag;
        window_.push_back(tag);
        while (window_.size() > window_len_) {
            window_sum_ -= window_.at(0);
            window_.pop_front();
        }
        if (!trigger_) {
            if (window_sum_ >= 0.9 * window_len_) {
                trigger_ = true;
                return 'B';
            } else {
                return 'N';
            }
        } else {
            if (window_sum_ <= 0.1 * window_len_) {
                trigger_ = false;
                return 'E';
            } else {
                return 'N';
            }
        }  
    }      
}






