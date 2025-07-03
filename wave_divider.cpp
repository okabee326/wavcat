#include "wavcat.h"

WaveDividerBase::WaveDividerBase(int64_t _divide_num, int64_t _samples, int64_t _channels)
{
    divide_num = _divide_num;
    samples = _samples;
    channels = _channels;

    output_channels = 0;
    output_samples = 0;
}

int64_t WaveDividerBase::OutputChannels()
{
    return output_channels;
}

int64_t WaveDividerBase::OutputSamples()
{
    return output_samples;
}

void WaveDividerBase::SetParam(int64_t num, int64_t sam, int64_t ch)
{
    divide_num = num;
    samples = sam;
    channels = ch;
}

void WaveDivider::Calc()
{
    //  |sample|channel|num||outsample|outchannel|over|
    //  |  1024|      2|  2||      512|         4|   0|
    //  |  1025|      2|  2||      513|         4|   1|
    //  |  1027|      2|  4||      257|         8|   1|

    output_samples = samples / divide_num;
    int64_t remind = samples % divide_num;
    if (remind != 0)
    {
        output_samples++;
    }

    //printf("%d -> %d\n", samples, output_samples);

    output_channels = channels * divide_num;
    output_remind = remind;
}

void WaveDivider::Divide(double **in, double **out)
{
    // 1024 div 2 => 0-511, 512-1023
    // 1025 div 2 => 0-512, 513-1024, skip 1025
    // 1025 div 4 => 0-256, 257-513, 514-770, 771-1024, skip 1025-1027
    // channel: 2, div 4 => c0a, c0b, c0c, c0d, c1a, c1b, c1c, c1d

    for (int c = 0; c < channels; c++)
    {
        for (int i = 0; i < divide_num; i++)
        {
            int64_t copysize = output_samples;


            if ((i + 1) * output_samples > samples)
            {
                copysize = samples - i * output_samples;
            }

            //printf("%d:%d => %ld\n", c, i, copysize);

            memcpy(out[c * divide_num + i], &in[c][i * output_samples], sizeof(double) * copysize);
        }
    }
}

void WaveCatenater::Calc()
{
    //  |sample|channel|num||outsample|outchannel|
    //  |   512|      4|  2||     1024|         2|
    //  |   513|      4|  2||     1026|         2|
    //  |   257|      8|  4||     1028|         2|

    output_channels = channels / divide_num;
    output_samples = samples * divide_num;
}

void WaveCatenater::Catenate(double **in, double **out)
{
    for (int c = 0; c < output_channels; c++)
    {
        for (int i = 0; i < divide_num; i++)
        {
            memcpy(&out[c][samples * i], in[c * divide_num + i], sizeof(double) * samples);
        }
    }
}