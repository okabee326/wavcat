#include "wave_writer.h"

WaveWriter::WaveWriter(std::string filename, WaveFormat _format, int64_t _blocksize)
{
    format = _format;
    blocksize = _blocksize;

    wf = fopen(filename.c_str(), "wb");

    if (wf == NULL)
    {
        printf("write file open error.\n");
    }
}

WaveWriter::~WaveWriter()
{
    if (wf != NULL)
    {
        fclose(wf);
    }
}

int64_t WaveWriter::WriteHeader()
{
    int outputbit = format.bytepersample * 8;
    int channels = format.channels;
    int outsampling = format.freq;

    if(wf == NULL){
        // file open error
        return -1;
    }

    // make wave header
    fputc('R', wf);
    fputc('I', wf);
    fputc('F', wf);
    fputc('F', wf);
    // int64_t datasize = (DSDsamplecount) / DSDrate * samplingrate * channels * outputbit/8;
    int64_t datasize = (format.samples) * (format.bytepersample) * channels;
    // printf("ceil(%lu): %lu\n",(int64_t)len, (int64_t)ceil(len / (double)BlockBytes));
    // printf("datasize: %lu\n", datasize);
    int64_t filesize = datasize + 8 + 28;
    write4bytes(wf, (int)filesize);
    fputc('W', wf);
    fputc('A', wf);
    fputc('V', wf);
    fputc('E', wf);
    fputc('f', wf);
    fputc('m', wf);
    fputc('t', wf);
    fputc(' ', wf);
    write4bytes(wf, (int)16);
    if (outputbit == 32 || outputbit == 64)
    {
        write2bytes(wf, (unsigned short)format.type);
    }
    else
    {
        write2bytes(wf, (unsigned short)1);
    }
    write2bytes(wf, (unsigned short)channels);
    write4bytes(wf, (int)outsampling);
    write4bytes(wf, (int)outsampling * channels * outputbit / 8);
    write2bytes(wf, (unsigned short)channels * outputbit / 8);
    write2bytes(wf, (unsigned short)outputbit);
    fputc('d', wf);
    fputc('a', wf);
    fputc('t', wf);
    fputc('a', wf);
    write4bytes(wf, (int64_t)datasize);

    return 0;
}

int64_t WaveWriter::WriteWave(double ***in, int64_t totalsize, int64_t *wrote_size)
{
    // file open check
    if(wf == NULL){
        return -1;
    }


    int64_t datasize = (format.samples) * (format.bytepersample);

    int64_t pCounter = 0;
    int64_t WriteBufferPointer = 0;
    unsigned char *WriteBuffer;
    // 32bit float output
    float *Floatdata;
    double *Doubledata;

    int channels = format.channels;
    int bytes_per_sample = format.bytepersample;
    int outputbit = bytes_per_sample * 8;

    int64_t writesize = blocksize;

    int64_t wrote_tmp = *wrote_size;

    //printf("type: %d\n", format.type);
    //printf("bit: %d\n", outputbit);
    //printf("channels: %d\n", channels);

    // double dither_size = static_cast<double>(dither_size_128);

    if (wrote_tmp + blocksize > totalsize)
    {
        writesize = totalsize - wrote_tmp;
        //printf("writesize: %lld\n", writesize);
    }

    if (format.type == 1)
    {
        WriteBuffer = (unsigned char *)malloc(sizeof(unsigned char) * blocksize * channels * bytes_per_sample);
        memset(WriteBuffer, 0, sizeof(unsigned char) * blocksize * channels * bytes_per_sample);
    }
    // 32bit float
    if (format.type == 3 && outputbit == 32)
    {
        Floatdata = (float *)malloc(blocksize * channels * sizeof(float));
        memset(Floatdata, 0, blocksize * channels * sizeof(float));
    }
    // 64bit float
    if (format.type == 3 && outputbit == 64)
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

            if (outputbit == 32 && format.type == 3)
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
                        //printf("check1: ch=%d, pos=%d\n", iChannel, write_count);
                        doubleResult = (*in)[iChannel][write_count];
                    }
                }
            }

            // change to PCM value
            if (format.type == 1)
            {
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
                        
                        val = val;

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

                        val = (*in)[iChannel][write_count];

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
            if (format.type == 3)
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

        (*wrote_size)++;
        write_count++;

    } // while(blocksize)

    if (format.type == 3)
    {
        if (outputbit == 64)
        {
            //printf("check2\n");
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

    if (outputbit == 32 && format.type == 3)
    {
        free(Floatdata);
    }
    if (outputbit == 64 && format.type == 3)
    {
        free(Doubledata);
    }
    if (format.type == 1)
    {
        free(WriteBuffer);
    }

    return write_count;
}