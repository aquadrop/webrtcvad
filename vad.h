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
    Vad(int mode = 3); 
    virtual ~Vad();
    void SetMode(int mode);
    bool IsSpeech(const int16_t *data, int num_point_per_frame, int sample_rate);
    char GetFrameState(const int16_t *data, int frame_len, int sample_rate);
private:
    int mode_;
    VadInst* handle_;
    bool trigger_;
    std::deque<int> window_;
    int window_len_;
    int window_sum_;
};


#endif
