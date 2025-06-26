#ifndef __WAVE_READER_H__
#define __WAVE_READER_H__

#include <cstdint>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdfloat>
#include <math.h>

// quad float
#ifdef __WIN64
  #define QUADFLOAT __float128
  #define QUAD_POW powq
  #define QUAD_ONE 1.0q
  #define QUAD_SPRINTF quadmath_snprintf
  #define QUAD_STRFORMAT "%.14Qf"
  #include <quadmath.h>
#else
  #define QUADFLOAT _Float128
  #define QUAD_POW powf128
  #define QUAD_ONE 1.0f128
  #define QUAD_SPRINTF strfromf128
  #define QUAD_STRFORMAT "%.14f"
#endif

typedef struct
{
    int64_t samples;
    int64_t datalen;
    int channels;
    int bytepersample;
    int freq;
    int type;

} WaveFormat;

class WaveReader{
    private:
        int blocksize; 
        FILE *f;
        WaveFormat format;
        
        int64_t loaded_size;

        bool isFileOpen = false;

        int LoadHeader();
        int64_t LoadWave(unsigned int optional_size, int option_peakcheck);

    public:
        WaveReader(int _blocksize);
        ~WaveReader();

        int Init(char* filename);
        int End();

        static double** CreateBuffer(int ch, int64_t size);
        static void FreeBuffer(int ch, double** buf);

        int64_t Load();
        int64_t GetBufferSize();
        int64_t GetDataLen();
        int64_t GetSamples();
        int64_t GetChannels();
        int64_t GetBytePerSample();
        int GetFreq();

        double **wave;
};

int64_t read4bytes(FILE *f);
unsigned read2bytes(FILE *f);
void write2bytes(FILE *f, unsigned short d);
void write4bytes(FILE *f, int64_t d);

#endif