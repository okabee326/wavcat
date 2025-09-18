#ifndef __WAVE_COMPARE_H__
#define __WAVE_COMPARE_H__

#include <string>

#include "wave_reader.h"
#include "wave_writer.h"

class WaveCompare{
    protected:
        double** data1;
        double** data2;
        double** data_diff;

        int channels;
        int64_t samples;
        int64_t datalen;
        int blocksize;

        WaveReader *reader1, *reader2;
        WaveWriter *writer;

        void Load(char* file1, char* file2, char* diff_file);
    public:
        WaveCompare(char*, char*, char*);
        ~WaveCompare();

        int Compare();
};

#endif