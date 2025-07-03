#ifndef __WAVCAT_H__
#define __WAVCAT_H__

#include "wave_reader.h"
#include "wave_writer.h"

class WaveDividerBase{
    protected:
        int64_t divide_num;
        int64_t samples;
        int64_t channels;

        int64_t output_channels;
        int64_t output_samples;
        int64_t output_remind;

    public:
        WaveDividerBase(int64_t _divide_num, int64_t _samples, int64_t _channels);

        virtual void Calc() {};

        int64_t OutputChannels();
        int64_t OutputSamples();

        void SetParam(int64_t num, int64_t sam, int64_t ch);
};

class WaveDivider : public WaveDividerBase {
    private:
        //
    public:
        void Calc();
        void Divide(double** in, double** out);
        using WaveDividerBase::WaveDividerBase;
};

class WaveCatenater : public WaveDividerBase{
    private:
        //
    public:
    void Calc();
    void Catenate(double** in, double** out);
    using WaveDividerBase::WaveDividerBase;
};

#endif