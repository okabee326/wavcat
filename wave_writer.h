#ifndef __WAVE_WRITER_H__
#define __WAVE_WRITER_H__

#include "wave_reader.h"
#include <string>

class WaveWriter {
    private:
        WaveFormat format;
        int64_t blocksize;
        FILE* wf;
    public:
        WaveWriter(std::string filename, WaveFormat format, int64_t blocksize);
        ~WaveWriter();
        int64_t WriteHeader();
        int64_t WriteWave(double*** in, int64_t totalsize, int64_t pos, int64_t* wrote_size);
};

#endif // __WAVE_WRITER_H__
