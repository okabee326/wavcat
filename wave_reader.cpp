#include "wave_reader.h"

//-------------------------------
// utility functions
//-------------------------------
int64_t read4bytes(FILE *f)
{
    unsigned char buf[4];

    if (fread(buf, 4, 1, f) != 1)
    {
        fprintf(stderr, "Read error\n");
        exit(1);
    }
    return ((256LU * buf[3] + buf[2]) * 256LU + buf[1]) * 256LU + buf[0];
}

unsigned read2bytes(FILE *f)
{
    unsigned char buf[2];

    if (fread(buf, 2, 1, f) != 1)
    {
        fprintf(stderr, "Read error\n");
        exit(1);
    }
    return 256U * buf[1] + buf[0];
}

void write2bytes(FILE *f, unsigned short d)
{
    unsigned char buf[2];
    buf[0] = (unsigned char)(d & 0xFF);
    d = d >> 8;
    buf[1] = (unsigned char)(d & 0xFF);
    int ret = fwrite(buf, 1, sizeof(buf), f);
    if (ret != 2)
        fprintf(stderr, "Write error\n");
    return;
}
void write4bytes(FILE *f, int64_t d)
{
    unsigned char buf[4];
    buf[0] = (unsigned char)(d & 0xFF);
    d = d >> 8;
    buf[1] = (unsigned char)(d & 0xFF);
    d = d >> 8;
    buf[2] = (unsigned char)(d & 0xFF);
    d = d >> 8;
    buf[3] = (unsigned char)(d & 0xFF);
    int ret = fwrite(buf, 1, sizeof(buf), f);
    if (ret != 4)
        fprintf(stderr, "Write error\n");
    return;
}

//------------------------------------
// WaveReader Class
//------------------------------------

int WaveReader::Init(char *filename)
{
    f = fopen(filename, "rb");

    if (f == NULL)
    {
        printf("file open error.\n");
        return 0;
    }

    isFileOpen = true;

    LoadHeader();

    // create buffer
    //orig
    /*
    wave = (double **)malloc(sizeof(double *) * format.channels);
    for (int i = 0; i < format.channels; i++)
    {
        wave[i] = (double *)malloc(sizeof(double) * (blocksize));
        memset(wave[i], 0, sizeof(double) * (blocksize));
    }
    */

    //new
    wave = new double*[format.channels];
    for(int i=0;i<format.channels;i++){
        wave[i] = new double[blocksize];
        memset(wave[i], 0, sizeof(double) * (blocksize));
    }

    return 0;
}

int64_t WaveReader::GetBufferSize(){
    return sizeof(unsigned char) * blocksize * format.bytepersample;
}

int64_t WaveReader::GetSamples(){
    return format.samples;
}

int64_t WaveReader::GetDataLen(){
    return format.datalen;
}

int64_t WaveReader::GetChannels(){
    return format.channels;
}

int64_t WaveReader::GetBytePerSample(){
    return format.bytepersample;
}

double** WaveReader::CreateBuffer(int ch, int64_t size){
    double** out = (double **)malloc(sizeof(double *) * ch);
    for (int i = 0; i < ch; i++)
    {
        out[i] = (double *)malloc(sizeof(double) * size);
        memset(out[i], 0, sizeof(double) * size);
    }

    return out;
}

void WaveReader::FreeBuffer(int ch, double** buf){
    for (int i = 0; i < ch; i++)
    {
        free(buf[i]);
    }
    free(buf);
}


int64_t WaveReader::Load(){
    int64_t size = LoadWave(0, 0);

    return size;
}

int WaveReader::End()
{
    if (isFileOpen)
    {
        fclose(f);
        isFileOpen = false;
    }

    if(wave != NULL){
        delete[] wave;
    }


    return 0;
}

WaveReader::WaveReader(int _blocksize)
{
    blocksize = _blocksize;
    wave = NULL;
}

WaveReader::~WaveReader()
{
    End();
}

int WaveReader::LoadHeader()
{
    unsigned char s[10];

    if (fread(s, 4, 1, f) != 1)
    {
        printf("Read error\n");
        fclose(f);
        return -1;
    }
    if (memcmp(s, "RIFF", 4) != 0)
    {
        printf("Not a RIFF format\n");
        fclose(f);
        return -1;
    }

    read4bytes(f);

    if (fread(s, 8, 1, f) != 1)
    {
        printf("Read error\n");
        fclose(f);
        return -1;
    }
    if (memcmp(s, "WAVEfmt ", 8) != 0)
    {
        printf("Not a WAVEfmt format\n");
        fclose(f);
        return -1;
    }

    int64_t len;

    // Chunk Length?
    len = read4bytes(f);
    if (len < 16)
    {
        printf("Length of WAVEfmt must be 16\n");
        return -1;
    }
    unsigned int type = read2bytes(f);
    int channels = read2bytes(f);
    int SamplingRate = read4bytes(f);
    read4bytes(f);
    int BytesPerSample = read2bytes(f);
    int bits = read2bytes(f);
    if (len != 16)
    {
        fseek(f, len - 16, SEEK_CUR);

        if (fread(s, 4, 1, f) != 1)
        {
            printf("error in fread in header.\n");
            return -1;
        }
        if (memcmp(s, "fact", 4) == 0)
        {

            len = read4bytes(f);
            fseek(f, len, SEEK_CUR);
        }
        else
        {
            fseek(f, -4L, SEEK_CUR);
        }
    }

    // read until 'data' chunk
    while (fread(s, 4, 1, f) == 1)
    {
        len = read4bytes(f);
        s[4] = 0;
        if (memcmp(s, "data", 4) == 0)
        {
            break;
        }
        for (int64_t i = 0; i < len; i++)
        {
            fgetc(f);
        }
    }

    format.samples = len / BytesPerSample;
    format.datalen = len;
    format.bytepersample = BytesPerSample;
    format.freq = SamplingRate;
    format.channels = channels;
    format.type = type;

    return 0;
}

// int loadWave(char* filename, double*** out, WaveFormat* format, unsigned int optional_size, int option_peakcheck){
//  return loading size.
int64_t WaveReader::LoadWave(unsigned int optional_size, int option_peakcheck)
{
    int64_t len = format.datalen;
    int type = format.type;
    int channels = format.channels;

    int byte_per_samples = format.bytepersample;

    int64_t load_samples = blocksize;
    int64_t load_size = blocksize * byte_per_samples;

    unsigned char* p;
    p = (unsigned char*)malloc(sizeof(unsigned char) * load_size);

    memset(p, 0, sizeof(unsigned char) * load_size);

    if (loaded_size + load_size > len)
    {
        load_size = len - loaded_size;
    }

    if (load_size > 0)
    {
        // if loaded_size is larger than wav file length, then read_size is negative.
        int cnt = fread(p, load_size, 1, f);

        if ((cnt != 1))
        { // p has wave format data, 16bit
            printf("error in fread.\nlen: %ld, load_size: %ld, loaded_size: %ld\n", len, load_size, loaded_size);
            return 0;
        }
    }
    else
    {
        return 0;
    }

    int64_t pCounter = 0;
    int64_t count = 0;

    int64_t samples = len / byte_per_samples;

    // wave to array

    while (pCounter < load_size)
    {
        for (int c = 0; c < channels; c++)
        {
            unsigned char H = 0x00;
            unsigned char M = 0x00;
            unsigned char L = 0x00;
            unsigned char H2 = 0x00;
            int64_t PCMvalue;
            int64_t temp_value;
            double signal;

            // type == 0xFE or 0xFF, then this is maybe integer pcm
            if (type != 3)
            {
                // 16bit
                if (byte_per_samples == 4)
                {
                    L = *(p + pCounter + 0 + 0 + c * byte_per_samples / channels); // read actual data
                    M = *(p + pCounter + 1 + c * byte_per_samples / channels);
                    PCMvalue = M * 256L + L;

                    double devide_by = 32768.0;
                    if (option_peakcheck != 0)
                    {
                        devide_by = 32767.0;
                    }
                    if (PCMvalue < 0x8000)
                    {
                        signal = ((double)(PCMvalue) / devide_by);
                    }
                    else
                    {
                        temp_value = (((~PCMvalue) + 1) & 0x0000FFFF);
                        signal = -1.0 * ((double)temp_value / devide_by);
                    }
                }
                // 24bit
                if (byte_per_samples == 6)
                {
                    L = *(p + pCounter + 0 + c * byte_per_samples / channels); // read actual data
                    M = *(p + pCounter + 1 + c * byte_per_samples / channels);
                    H = *(p + pCounter + 2 + c * byte_per_samples / channels);

                    PCMvalue = H * 256L * 256L + M * 256L + L;

                    double devide_by = 8388608.0;
                    if (option_peakcheck != 0)
                    {
                        devide_by = 8388607.0;
                    }

                    if (PCMvalue < 0x800000)
                    {
                        signal = ((double)(PCMvalue) / devide_by);
                    }
                    else
                    {
                        temp_value = (((~PCMvalue) + 1) & 0x00FFFFFF);
                        signal = -1.0 * ((double)temp_value / devide_by);
                    }
                }
                // 32bit integer
                if (byte_per_samples == 8)
                {
                    L = *(p + pCounter + 0 + c * byte_per_samples / channels); // read actual data
                    M = *(p + pCounter + 1 + c * byte_per_samples / channels);
                    H = *(p + pCounter + 2 + c * byte_per_samples / channels);
                    H2 = *(p + pCounter + 3 + c * byte_per_samples / channels);

                    PCMvalue = H2 * 256L * 256L * 256L + H * 256L * 256L + M * 256L + L;

                    double devide_by = 2147483648.0;
                    if (option_peakcheck != 0)
                    {
                        devide_by = 2147483647.0;
                    }

                    if (PCMvalue < 0x80000000)
                    {
                        signal = ((double)(PCMvalue) / devide_by);
                    }
                    else
                    {
                        temp_value = (((~PCMvalue) + 1) & 0xFFFFFFFF);
                        signal = -1.0 * ((double)temp_value / devide_by);
                    }
                }
                // 64bit integer
                if (byte_per_samples == 16)
                {
                    PCMvalue = *((int64_t *)(p + pCounter + c * byte_per_samples / channels));

#ifdef __WIN64
                    __float128 devide_by = 9223372036854775808.0q;
#else
                    _Float128 devide_by = 9223372036854775808.0f128;
#endif

                    if (option_peakcheck != 0)
                    {
#ifdef __WIN64
                        devide_by = 9223372036854775807.0q;
#else
                        devide_by = 9223372036854775807.0f128;
#endif
                    }

                    QUADFLOAT val_temp = 0.0;
                    if (PCMvalue < 0x8000000000000000)
                    {
                        val_temp = (QUADFLOAT)(PCMvalue);
                        val_temp = (val_temp / devide_by);
                        signal = (double)val_temp;
                    }
                    else
                    {
                        temp_value = (((~PCMvalue) + 1) & 0xFFFFFFFFFFFFFFFF);
                        signal = static_cast<double>(((QUADFLOAT)temp_value / devide_by) * -1.0);
                    }
                    // printf("%ld, %f\n", PCMvalue, signal);
                }
            }
            else
            {
                // 32bit float
                if (byte_per_samples == 8)
                {
                    signal = *((float *)(p + pCounter + c * byte_per_samples / channels));
                }
                // 64bit float
                if (byte_per_samples == 16)
                {
                    signal = *((double *)(p + pCounter + c * byte_per_samples / channels));
                }
            }

            wave[c][count] = signal;
            // printf("%f\n", wave[c][count]);
        }

        pCounter += byte_per_samples;
        count++;
    }

    // printf("%1.10f\n", wave[0][1000]);

    free(p);

    loaded_size += load_size;

    return load_size;
}

//-----------------------------------------------
//
//-----------------------------------------------

int writeWaveHeader(FILE *wf, int64_t size, int type, int outputbit, int outsampling, int channels, int64_t addtional_samples)
{
    // make wave header
    fputc('R', wf);
    fputc('I', wf);
    fputc('F', wf);
    fputc('F', wf);
    // int64_t datasize = (DSDsamplecount) / DSDrate * samplingrate * channels * outputbit/8;
    int64_t datasize = (size + addtional_samples) * (outputbit / 8) * channels;
    // printf("ceil(%lu): %lu\n",(int64_t)len, (int64_t)ceil(len / (double)BlockBytes));
    // printf("datasize: %lu\n", datasize);
    int64_t filesize = datasize + 8 + 28;

    // todo: 64bit -> 32bit cast?
    write4bytes(wf, (int64_t)filesize);
    fputc('W', wf);
    fputc('A', wf);
    fputc('V', wf);
    fputc('E', wf);
    fputc('f', wf);
    fputc('m', wf);
    fputc('t', wf);
    fputc(' ', wf);
    write4bytes(wf, (int64_t)16);
    if (outputbit == 32 || outputbit == 64)
    {
        write2bytes(wf, (unsigned short)type);
    }
    else
    {
        write2bytes(wf, (unsigned short)1);
    }
    write2bytes(wf, (unsigned short)channels);
    write4bytes(wf, (int64_t)outsampling);
    write4bytes(wf, (int64_t)outsampling * channels * outputbit / 8);
    write2bytes(wf, (unsigned short)channels * outputbit / 8);
    write2bytes(wf, (unsigned short)outputbit);
    fputc('d', wf);
    fputc('a', wf);
    fputc('t', wf);
    fputc('a', wf);
    write4bytes(wf, (int64_t)datasize);

    return 0;
}

int writeWave(FILE *wf, double ***in, int64_t blocksize, int64_t totalsize, int64_t *wrote_size, int type, int outputbit, int outsampling, int channels, int option_outputround,
              int64_t addtional_samples)
{
    int64_t datasize = (totalsize + addtional_samples) * (outputbit / 8) * channels;

    int64_t pCounter = 0;
    int64_t WriteBufferPointer = 0;
    unsigned char *WriteBuffer;
    // 32bit float output
    float *Floatdata;
    double *Doubledata;

    int64_t writesize = blocksize;

    int64_t wrote_tmp = *wrote_size;

    if (wrote_tmp + blocksize > totalsize + addtional_samples)
    {
        writesize = totalsize + addtional_samples - wrote_tmp;
        printf("writesize: %lld\n", writesize);
    }

    if (type == 1)
    {
        WriteBuffer = (unsigned char *)malloc(sizeof(unsigned char) * blocksize * channels * outputbit / 8);
        memset(WriteBuffer, 0, sizeof(unsigned char) * blocksize * channels * outputbit / 8);
    }
    if (type == 3 && outputbit == 32)
    {
        Floatdata = (float *)malloc(blocksize * channels * sizeof(float));
        memset(Floatdata, 0, blocksize * channels * sizeof(float));
    }
    if (type == 3 && outputbit == 64)
    {
        Doubledata = (double *)malloc(blocksize * channels * sizeof(double));
        memset(Doubledata, 0, blocksize * channels * sizeof(double));
    }

    double wav_error[2] = {0.0};
    double wav_buffer[2] = {0.0};

    int64_t write_count = 0;

    while (write_count < writesize)
    {
        for (int iChannel = 0; iChannel < channels; iChannel++)
        {
            unsigned char H = 0x00;
            unsigned char H2 = 0x00;
            unsigned char M = 0x00;
            unsigned char L = 0x00;
            int64_t PCMvalue;
            float oneResult;
            double doubleResult;
            double dither_fir = 0.0;

            if (outputbit == 32 && type == 3)
            {
                if (wrote_tmp + write_count >= totalsize)
                {
                    oneResult = 0.0;
                }
                else
                {
                    oneResult = (float)(*in)[iChannel][write_count];
                }
            }
            else
            {
                if (outputbit == 64)
                {
                    if (wrote_tmp + write_count >= totalsize)
                    {
                        doubleResult = 0.0;
                    }
                    else
                    {
                        doubleResult = (*in)[iChannel][write_count];
                    }
                }
            }

            // change to PCM value
            if (type == 1)
            {
                // PCMvalue = Shaper_int[iChannel*size + j ];
                int64_t max = 4294967296 - 1;

                double temp, delta;
                double errors = 0.0;

                // write PCM value to file

                if (outputbit == 64)
                {
                    if (wrote_tmp + write_count >= totalsize)
                    {
                        PCMvalue = 0;
                    }
                    else
                    {
                        double val;
                        double dither = 0.0;

                        val = (*in)[iChannel][write_count];

                        if (val < 0.0)
                        {
                            QUADFLOAT val_temp = (QUADFLOAT)(fabs(val));
#ifdef __WIN64
                            val_temp = (val_temp * 9223372036854775807.0q);
#else
                            val_temp = (val_temp * 9223372036854775807.0f128);
#endif

                            PCMvalue = static_cast<int64_t>(val_temp);
                            PCMvalue = (~PCMvalue) + 1;
                        }
                        else
                        {
                            if (val == 0.0)
                            {
                                PCMvalue = 0;
                            }
                            else
                            {
                                QUADFLOAT val_temp = (QUADFLOAT)(val);
#ifdef __WIN64
                                val_temp = (val_temp * 9223372036854775807.0q);
#else
                                val_temp = (val_temp * 9223372036854775807.0f128);
#endif

                                PCMvalue = static_cast<int64_t>(val_temp);
                            }
                        }
                    }

                    memcpy(&WriteBuffer[WriteBufferPointer], &PCMvalue, sizeof(int64_t));
                    WriteBufferPointer += 8;

                    // printf("%ld, %ld\n", PCMvalue, *(int64_t*)( &WriteBuffer[WriteBufferPointer-8]) );
                }

                if (outputbit == 32)
                {
                    max = 4294967295;
                    if (wrote_tmp + write_count >= totalsize)
                    {
                        PCMvalue = 0;
                    }
                    else
                    {
                        double val = (*in)[iChannel][write_count];
                        double val_dither = val * 2147483648.0;
                        double dither = 0.0;

                        // TwoSum(val, dither, &temp, &delta);
                        // errors += delta;
                        // errors = delta;

                        // val = temp + errors;
                        val = val + dither;

                        // negative
                        if (val < 0.0)
                        {

                            val = fabs(val * 2147483648.0);
                            if (val > 2147483648.0)
                            {
                                val = 2147483648.0;
                            }

                            PCMvalue = (int64_t)(val);
                            PCMvalue = (~PCMvalue) + 1;

                            wav_buffer[iChannel] = val;
                        }
                        else
                        {
                            if (val == 0.0)
                            {
                                PCMvalue = 0;
                                wav_buffer[iChannel] = val;
                            }
                            // positive
                            else
                            {
                                val = val * 2147483648.0;
                                if (val > 2147483647.0)
                                {
                                    val = 2147483647.0;
                                }

                                PCMvalue = (int64_t)(val);

                                wav_buffer[iChannel] = val;
                            }
                        }
                    }

                    L = (PCMvalue & 0xFF);
                    M = (PCMvalue >> 8) & 0xFF;
                    H = (PCMvalue >> 16) & 0xFF;
                    H2 = (PCMvalue >> 24) & 0xFF;

                    WriteBuffer[WriteBufferPointer] = L;
                    WriteBufferPointer++;
                    WriteBuffer[WriteBufferPointer] = M;
                    WriteBufferPointer++;
                    WriteBuffer[WriteBufferPointer] = H;
                    WriteBufferPointer++;
                    WriteBuffer[WriteBufferPointer] = H2;
                    WriteBufferPointer++;
                }
                if (outputbit == 24)
                {
                    max = 16777215;
                    if (wrote_tmp + write_count >= totalsize)
                    {
                        PCMvalue = 0;
                    }
                    else
                    {
                        double val = (*in)[iChannel][write_count];
                        // printf("%f\n", val);
                        double val_dither = val * 8388608.0;
                        double dither = 0.0;

                        // printf("%3.14f\n", dither);

                        val = val + dither;

                        /// TwoSum(val, dither, &temp, &delta);
                        // errors += delta;
                        // errors = delta;
                        // val = temp + errors;

                        if (val < 0.0)
                        {
                            val = fabs(val * 8388608.0);
                            if (val > 8388608.0)
                            {
                                val = 8388608.0;
                            }

                            PCMvalue = (int64_t)(val);
                            PCMvalue = (~PCMvalue) + 1;

                            wav_buffer[iChannel] = val;
                        }
                        else
                        {
                            if (val == 0.0)
                            {
                                PCMvalue = 0;
                                wav_buffer[iChannel] = val;
                            }
                            else
                            {
                                val = val * 8388608.0;
                                if (val > 8388607.0)
                                {
                                    val = 8388607.0;
                                }

                                PCMvalue = (int64_t)(val);

                                wav_buffer[iChannel] = val;
                            }
                        }
                    }
                    L = (PCMvalue & 0xFF);
                    M = (PCMvalue >> 8) & 0xFF;
                    H = (PCMvalue >> 16) & 0xFF;

                    WriteBuffer[WriteBufferPointer] = L;
                    WriteBufferPointer++;
                    WriteBuffer[WriteBufferPointer] = M;
                    WriteBufferPointer++;
                    WriteBuffer[WriteBufferPointer] = H;
                    WriteBufferPointer++;
                }
                if (outputbit == 16)
                {
                    max = 65535;
                    if (wrote_tmp + write_count >= totalsize)
                    {
                        PCMvalue = 0;
                    }
                    else
                    {
                        double val;
                        double dither = 0.0;

                        val = (*in)[iChannel][write_count] + dither;

                        if (val < 0.0)
                        {
                            val = fabs(val * 32768.0);
                            if (val > 32768.0)
                            {
                                val = 32768;
                            }

                            PCMvalue = (int64_t)(val);
                            PCMvalue = (~PCMvalue) + 1;

                            wav_buffer[iChannel] = val;
                        }
                        else
                        {
                            if (val == 0.0)
                            {
                                PCMvalue = 0;
                                wav_buffer[iChannel] = val;
                            }
                            else
                            {
                                val = val * 32768.0;
                                if (val > 32767.0)
                                {
                                    val = 32767.0;
                                }

                                PCMvalue = (int64_t)(val);

                                wav_buffer[iChannel] = val;
                            }
                        }
                    }
                    L = (PCMvalue & 0xFF);
                    M = (PCMvalue >> 8) & 0xFF;

                    WriteBuffer[WriteBufferPointer] = L;
                    WriteBufferPointer++;
                    WriteBuffer[WriteBufferPointer] = M;
                    WriteBufferPointer++;
                }

            } // end if(type == 1)
            if (type == 3)
            {
                if (outputbit == 32)
                {
                    Floatdata[WriteBufferPointer] = oneResult;
                }
                else
                {
                    Doubledata[WriteBufferPointer] = doubleResult;
                }
                WriteBufferPointer++;
            }
        } // for(channels)

        *wrote_size++;
        write_count++;

    } // for(blocksize)

    if (type == 3)
    {
        if (outputbit == 64)
        {
            fwrite(Doubledata, 1, WriteBufferPointer * sizeof(double), wf);
        }
        else
        {
            fwrite(Floatdata, 1, WriteBufferPointer * sizeof(float), wf);
        }
    }
    else
    {
        fwrite(WriteBuffer, 1, WriteBufferPointer, wf);
    }

    // fclose(wf);

    if (outputbit == 32 && type == 3)
    {
        free(Floatdata);
    }
    if (outputbit == 64 && type == 3)
    {
        free(Doubledata);
    }
    if (type == 1)
    {
        free(WriteBuffer);
    }

    return write_count;
}
