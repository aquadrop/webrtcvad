#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>

#include "test/parse-option.h"
#include "test/wav.h"
#include "vad.h"

int main(int argc, char *argv[]) {
    const char *usage = "Apply energy vad for input wav file\n"
                        "Usage: vad-test wav_in_file\n";
    ParseOptions po(usage);

    int frame_len = 30; // 10 ms
    int window_len = 300; // 300 ms
    po.Register("frame-len", &frame_len, "frame length in millionsenconds, must be 10/20/30");
    int mode = 3; 
    po.Register("mode", &mode, "vad mode");

    po.Read(argc, argv);

    if (po.NumArgs() != 2) {
        po.PrintUsage();
        exit(1);
    }

    std::string wav_in = po.GetArg(1), 
         wav_out = po.GetArg(2);

    WavReader reader(wav_in.c_str());

    printf("input file %s info: \n"
          "sample_rate %d \n"
          "channels %d \n"
          "bits_per_sample_ %d \n",
          wav_in.c_str(),
          reader.SampleRate(), 
          reader.NumChannel(),
          reader.BitsPerSample());

    int num_channel = reader.NumChannel();
    int sample_rate = reader.SampleRate();
    int num_sample = reader.NumSample();
    int bits_per_sample = reader.BitsPerSample();
    int num_point_per_frame = (int)(frame_len * sample_rate / 1000);
    printf("num_point_per_frame %d\n", num_point_per_frame);
    printf("num_sample %d\n", num_sample);
    short *data = (short *)calloc(sizeof(short), num_sample);
    // Copy first channel
    for (int i = 0; i < num_sample; i++) {
        data[i] = reader.Data()[i * reader.NumChannel()];
        // printf("data[%d] %d\n",i, data[i]);
    }

    printf("mode %d\n", mode);
    printf("frame len %d\n", frame_len);
    Vad vad(mode, num_channel, sample_rate, bits_per_sample, window_len, frame_len);

    int num_frames = num_sample / num_point_per_frame;
    std::vector<char> vad_reslut;

    for (int i = 0; i < num_sample; i += num_point_per_frame) {
        // last frame 
        if (i + num_point_per_frame > num_sample) break;
        bool tag = vad.IsSpeech(data + i, frame_len, sample_rate);
        char state = vad.SlideWindow(tag);
        vad_reslut.push_back(state);
        std::cout << state;
    }
    std::cout << '\n';

    std::vector<int> vad_begin_idx;
    std::vector<int> vad_end_idx;
    for (int i = 0; i < vad_reslut.size(); i++) {
        if (vad_reslut[i] == 'B') {
            vad_begin_idx.push_back(i);
        } else if (vad_reslut[i] == 'E') {
            vad_end_idx.push_back(i);
        }
    }

    int num_speech_frames = 0;
    for (int i = 0; i < vad_end_idx.size(); i++) {
        num_speech_frames += vad_end_idx[i] - vad_begin_idx[i] + 1;
    }
    int num_speech_sample = num_speech_frames * num_point_per_frame;
    short *speech_data = (short *)calloc(sizeof(short), num_speech_sample);

    int speech_cur = 0;
    for (int i = 0; i < vad_end_idx.size(); i++) {
        for (int j = vad_begin_idx[i]; j < vad_end_idx[i]+1; j++) {
            memcpy(speech_data + speech_cur * num_point_per_frame,
                   data + j * num_point_per_frame, 
                   num_point_per_frame * sizeof(short));
            speech_cur++;
        }
    }

    // for (std::vector<int>::iterator iter = vad_begin_idx.begin(); iter != vad_begin_idx.end(); ++iter){
    //     std::cout << *iter << ' ';
    // }
    // std::cout << '\n';
    // for (std::vector<int>::iterator iter = vad_end_idx.begin(); iter != vad_end_idx.end(); ++iter){
    //     std::cout << *iter << ' ';
    // }
    // std::cout << '\n';
    
    WavWriter writer(speech_data, num_speech_sample, reader.NumChannel(), 
                        reader.SampleRate(), reader.BitsPerSample());

    writer.Write(wav_out.c_str());
    free(data);
    free(speech_data);
    return 0;
}


