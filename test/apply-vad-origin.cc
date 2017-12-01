/* Created on 2017-03-01
 * Author: Binbin Zhang
 * 
 * Change on 2017-11-30 by cb
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>
#include <queue>
#include <iostream>

#include "test/parse-option.h"
#include "test/wav.h"
#include "vad.h"

using namespace std;

class ApplyVAD {
public:
    ApplyVAD(Vad vad, unsigned short num_channels, unsigned long sample_rate,
    unsigned short bits_per_sample, unsigned int window_size, unsigned int unit_size):
        vad(vad),
        num_channels(num_channels),
        sample_rate(sample_rate),
        bits_per_sample(bits_per_sample),
        window_size(window_size) {
            block_align = num_channels * bits_per_sample / 8;
            bytes_per_sec = num_channels * sample_rate * bits_per_sample / 8;
            bytes_per_unit = unit_size / 1000 * bytes_per_sec;
            buff_size = window_size / unit_size;
            num_data = bytes_per_unit / (bits_per_sample / 8);
            num_sample = num_data / num_channels;
            num_point_per_unit_frame = (int)(unit_size * sample_rate / 1000);
            num_unit_frames = buff_size;
        }

public:
    bool receive(vector<char> data) {
        vector<short> data_;
        for (int i = 0; i < num_data; i++) {
            switch (bits_per_sample) {
                case 8: {
                    char sample;
                    sample = data[i];
                    data_.push_back((short)sample);
                    break;
                }
                case 16: {
                    short sample;
                    char char0 = data[2*i];
                    char char1 = data[2*i+1];
                    sample = (char1 << 8) | (char0 & 0xFF);
                    data_.push_back((short)sample);
                    break;
                }
                case 32: {
                    int sample;
                    char char0 = data[4*i];
                    char char1 = data[4*i+1];
                    char char2 = data[4*i+2];
                    char char3 = data[4*i+3];
                    short short0 = (char3 << 8) | (char2 & 0xFF);
                    short short1 = (char1 << 8) | (char0 & 0xFF);
                    sample = (short1 << 16) | (short0 & 0xFFFF);
                    data_.push_back((short)sample);
                    break;
                }
                default:
                    cout << "unsupported quantization bits" << endl;
                    exit(1);
            }
        }
        vector<short> single_channel_data;
        for (int i = 0; i < num_sample; i++) {
            single_channel_data.push_back(data_[i * num_channels]);
        }
        buff.push_back(single_channel_data);
        if (buff.size() == buff_size)
            return true;
        else if (buff.size < buff_size)
            return false;
        else 
            buff.erase(buff.begin());
            return true;
    }

    int process(vector<char> data) {
        bool flag = receive(data);
        if (!flag)
            return 0; // hold state
        else
            vector<bool> vad_result;
            int num_speech_frames = 0;
            for (int i = 0; i < buff.size(); i++) {
                vector<short> tmp = buff[i];
                short* p_data = new short[tmp.size()];
                for (int i = 0; i < tmp.size(); i++)
                    p_data[i] = tmp[i];
                bool tags = vad.IsSpeech(p_data, num_point_per_unit_frame, sample_rate);
                // vad_result.push_back(tags);
                // std::cout<< "tags:" << tags << std::endl;
                if (tags) num_speech_frames++;
            }
            if (num_speech_frames >= num_unit_frames * 0.9)
                return 1;
            else
                return -1;
    }

private:
    unsigned short num_channels;
    unsigned long sample_rate;
    unsigned short bits_per_sample;
    unsigned int window_size; // ms
    unsigned int unit_size; // ms
    Vad vad;

    unsigned short block_align;
    unsigned int bytes_per_sec;
    unsigned int bytes_per_unit;
    int num_point_per_unit_frame;
    int num_unit_frames;
    int num_data;
    int num_sample;
    vector<vector<short>> buff;
    unsigned int buff_size;
};


int main(int argc, char *argv[]) {
    const char *usage = "Apply energy vad for input wav file\n"
                        "Usage: vad-test wav_in_file\n";
    ParseOptions po(usage);

    int frame_len = 10; // 10 ms
    po.Register("frame-len", &frame_len, "frame length in millionsenconds, must be 10/20/30");
    int mode = 0; 
    po.Register("mode", &mode, "vad mode");

    po.Read(argc, argv);

    if (po.NumArgs() != 2) {
        po.PrintUsage();
        exit(1);
    }

    std::string wav_in = po.GetArg(1), 
         wav_out = po.GetArg(2);
 
    WavReader reader(wav_in.c_str());

    //printf("input file %s info: \n"
    //       "sample_rate %d \n"
    //       "channels %d \n"
    //       "bits_per_sample_ %d \n",
    //       wav_in.c_str(),
    //       reader.SampleRate(), 
    //       reader.NumChannel(),
    //       reader.BitsPerSample());
    
    int sample_rate = reader.SampleRate();
    int num_sample = reader.NumSample();
    int num_point_per_frame = (int)(frame_len * sample_rate / 1000);
    std::cout << "num_point_per_frame:" << num_point_per_frame << std::endl;
   
    short *data = (short *)calloc(sizeof(short), num_sample);
    // Copy first channel
    for (int i = 0; i < num_sample; i++) {
        data[i] = reader.Data()[i * reader.NumChannel()];
    }

    printf("mode %d\n", mode);
    Vad vad(mode);

    int num_frames = num_sample / num_point_per_frame;
    std::vector<bool> vad_reslut;
    int num_speech_frames = 0;

    for (int i = 0; i < num_sample; i += num_point_per_frame) {
        // last frame 
        if (i + num_point_per_frame > num_sample) break;
        bool tags = vad.IsSpeech(data+i, num_point_per_frame, sample_rate);
        vad_reslut.push_back(tags);
        std::cout<< "tags:" << tags << std::endl;
        if (tags) num_speech_frames++;
        //printf("%f %d \n", float(i) / sample_rate, (int)tags);
    }

    int num_speech_sample = num_speech_frames * num_point_per_frame;
    short *speech_data = (short *)calloc(sizeof(short), num_speech_sample);
    
    int speech_cur = 0;
    for (int i = 0; i < vad_reslut.size(); i++) {
        // speech
        if (vad_reslut[i]) {
            memcpy(speech_data + speech_cur * num_point_per_frame,
                   data + i * num_point_per_frame, 
                   num_point_per_frame * sizeof(short));
            speech_cur++;
        }
    }

    WavWriter writer(speech_data, num_speech_sample, 1, 
                        reader.SampleRate(), reader.BitsPerSample());

    writer.Write(wav_out.c_str());
    free(data);
    free(speech_data);
    return 0;
}


