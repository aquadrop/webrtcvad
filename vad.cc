#include "vad.h"
#include "stdio.h"
#include "stdlib.h"
#include <string>
#include <fstream>
#include <iostream>

Vad::Vad(int mode, unsigned short num_channels, unsigned long sample_rate,
    unsigned short bits_per_sample, unsigned int window_duration_ms, unsigned int frame_duration_ms):
        mode_(mode),
        num_channels(num_channels),
        sample_rate(sample_rate),
        bits_per_sample(bits_per_sample),
        window_duration_ms(window_duration_ms),
        frame_duration_ms(frame_duration_ms) {
    if (WebRtcVad_Create(&handle_) < 0) {
        printf("Create webrtc vad handle error\n");
        exit(-1);
    }
    WebRtcVad_Init(handle_);
    WebRtcVad_set_mode(handle_, mode_);
    block_align = num_channels * bits_per_sample / 8;
    bytes_per_sec = num_channels * sample_rate * bits_per_sample / 8;
    bytes_per_unit = this->frame_duration_ms / 1000 * bytes_per_sec;
    window_len_ = this->window_duration_ms / this->frame_duration_ms;
    num_data = bytes_per_unit / (bits_per_sample / 8);
    num_sample = num_data / num_channels;
    this->num_point_per_frame = (int)(frame_duration_ms * sample_rate / 1000);
}

Vad::~Vad() {
    WebRtcVad_Free(handle_);
}

void Vad::SetMode(int mode) {
    mode_ = mode;
    WebRtcVad_set_mode(handle_, mode_);
}

char Vad::Process(const int16_t *data) {
    bool tag = this->IsSpeech(data, this->num_point_per_frame, this->sample_rate);
    return this->SlideWindow(tag);
}

void Vad::ResetWindow() {
    this->window_.clear();
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

char Vad::SlideWindow(int tag) {
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
            if (window_sum_ >= this->_VOICE_THRED_ * window_len_) {
                trigger_ = true;
                return 'B';
            } else {
                return 'N';
            }
        } else {
            if (window_sum_ <= (1 - this->_VOICE_THRED_) * window_len_) {
                trigger_ = false;
                return 'E';
            } else {
                return 'N';
            }
        }  
    }      
}

std::pair<int16_t*, int> Vad::ReadPcm(std::string pcm) {
    std::ifstream is(pcm.c_str());
    std::pair<int16_t*, int> pair;
    if (is) {
        // get length of file:
        is.seekg (0, is.end);
        int length = is.tellg();
        is.seekg (0, is.beg);

        char * buffer = new char [length];
        
        std::cout << "Reading " << length << " characters... ";
        is.read (buffer,length);

        short* new_buffer=(short*)buffer;

        if (is)
            std::cout << "all characters read successfully." << std::endl;
        else
            std::cout << "error: only " << is.gcount() << " could be read";
        is.close();
        
        pair.first = new_buffer;
        // char to short
        pair.second = length / 2;
        
    }

    return pair;
}




