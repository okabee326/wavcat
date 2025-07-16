#include "wavcat.h"

#ifndef __PRODUCTION__
#include <gtest/gtest.h>
#endif

void show_usage()
{
    printf("wavcat infile outfile\n");
}

int main(int argc, char *argv[])
{
    char *infile = NULL;
    char *outfile = NULL;

    int option_divide = 0;
    int option_cat = 0;
    int option_test = 0;
    int option_verification = 0;

    int divide_num = 0;
    int cat_num = 0;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            if (infile == NULL)
            {
                infile = argv[i];
                continue;
            }
            if (outfile == NULL)
            {
                outfile = argv[i];
                continue;
            }
        }

        #ifndef __PRODUCTION__
        if (strcmp(argv[i], "--test") == 0)
        {
            option_test = 1;
            continue;
        }
        #endif

        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--divide") == 0)
        {
            if (i + 1 != argc)
            {
                divide_num = atoi(argv[i + 1]);

                option_divide = 1;

                i++;
            }

            continue;
        }

        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cat") == 0)
        {
            if (i + 1 != argc)
            {
                divide_num = atoi(argv[i + 1]);

                option_cat = 1;

                i++;
            }

            continue;
        }

        if(strcmp(argv[i], "--veri") == 0){
            if (i + 1 != argc)
            {
                divide_num = atoi(argv[i + 1]);

                option_verification = 1;

                i++;
            }

            continue;
        }
    }

    if (infile == NULL)
    {
        show_usage();
        return 1;
    }

    if (outfile == NULL)
    {
        show_usage();
        return 1;
    }

    #ifndef __PRODUCTION__
    if (option_test == 1)
    {
        ::testing::InitGoogleTest(&argc, argv);

        return RUN_ALL_TESTS();
    }
    #endif

    if (option_divide == 1 || option_cat == 1 || option_verification == 1)
    {
        // divide mode
        if (divide_num == 0)
        {
            show_usage();

            return 1;
        }

        if (divide_num == 2 || divide_num == 4 || divide_num == 8)
        {
            // support 2 or 4 or 8
        }
        else
        {
            // no support
            show_usage();
            return 1;
        }
    } else {
        show_usage();
        return 1;
    }

    // load wave file
    int blocksize = 32768;
    int64_t loaded_size = 0;

    WaveReader *reader = new WaveReader(blocksize);

    reader->Init(infile);

    int64_t buffersize = reader->GetBufferSize();
    int64_t channels = reader->GetChannels();
    int64_t samples = reader->GetSamples();
    int64_t datalen = reader->GetDataLen();
    int64_t out_channels;
    int64_t out_samples;

    //printf("%d, %d, %d\n", divide_num, samples, channels);

    WaveDivider *divider = NULL;
    WaveCatenater *catenater = NULL;

    // create buffer
    double **in;
    double **out;
    double **v_out;

    if (option_divide == 1 || option_verification == 1)
    {
        divider = new WaveDivider(divide_num, samples, channels);
        divider->Calc();
        out_channels = divider->OutputChannels();
        out_samples = divider->OutputSamples();

        // input buffer
        in = WaveReader::CreateBuffer(channels, samples);
        out = WaveReader::CreateBuffer(out_channels, out_samples);

        if(option_verification == 1){
            v_out = WaveReader::CreateBuffer(channels, samples+1);

            catenater = new WaveCatenater(divide_num, out_samples, out_channels);
            catenater->Calc();
        }
    }

    if(option_cat == 1){
        catenater = new WaveCatenater(divide_num, samples, channels);
        catenater->Calc();
        out_channels = catenater->OutputChannels();
        out_samples = catenater->OutputSamples();

        // input buffer
        in = WaveReader::CreateBuffer(channels, samples);
        out = WaveReader::CreateBuffer(out_channels, out_samples);
    }

    // display variable
    printf("input samples:  %ld\n", samples);
    printf("output samples: %ld\n", out_samples);

    int count = 0;

    // read file
    while (loaded_size < datalen)
    {
        int64_t load_size = reader->Load();

        if(load_size == -1){
            printf("error\n");
            return 1;
        }

        int64_t loaded_samples = loaded_size / reader->GetBytePerSample();
        int64_t load_samples = load_size / reader->GetBytePerSample();
        loaded_size += load_size;

        //printf("%ld / %ld\n", load_size, loaded_size);

        // process
        // load wave
        for (int i = 0; i < channels; i++)
        {
            memcpy(&in[i][loaded_samples], reader->wave[i], sizeof(double) * load_samples);

            //printf("%3.14f\n", reader->wave[i][0]);
        }

        count++;
    }

    //printf("aaa\n");

    // process
    if(option_divide == 1 || option_verification == 1){
        divider->Divide(in, out);

        //printf("%3.14f\n", out[2][0]);
    }
    
    if(option_cat == 1){
        catenater->Catenate(in, out);
        //printf("%3.14f\n", in[2][0]);
        //printf("%3.14f\n", out[1][0]);
    }

    if(option_verification == 1){
        catenater->Catenate(out, v_out);
    }

    // write buffer
    WaveFormat write_format;

    write_format.bytepersample = 8; // 64bit float
    write_format.channels = out_channels;
    write_format.freq = reader->GetFreq();
    write_format.samples = out_samples;
    write_format.type = 3; // float

    int64_t write_datalen = 8 * out_channels * out_samples;
    write_format.datalen = write_datalen;
    int code = 0;

    WaveWriter *writer = new WaveWriter(outfile, write_format, blocksize);

    int header_result = writer->WriteHeader();

    if (header_result == -1)
    {
        // file open error
        code = 1;
    }

    int64_t written_size = 0;

    if (code == 0)
    {
        int count = 0;
        while (written_size < out_samples)
        {
            writer->WriteWave(&out, out_samples, count, &written_size);

            count += blocksize;
        }
    }

    // verification mode
    if(option_verification == 1){
        for(int i=0;i<100;i++){
            printf("%3.14f, %3.14f\n", in[0][samples/2+i*100-100], v_out[0][samples/2 + i*100-100]);
        }
    }

    delete reader;
    delete writer;

    // display variable
    /*
    for(int i=0;i<100;i++){
        printf("%3.14lf\n",in[0][samples / 2 + 1 + i*100]);
        printf("%3.14lf\n",out[1][i*100]);
    }
    */

    // release memory
    if (option_divide == 1 || option_verification == 1)
    {
        delete divider;
        WaveReader::FreeBuffer(channels, in);
        WaveReader::FreeBuffer(out_channels, out);
    }

    if(option_cat == 1){
        delete catenater;
        WaveReader::FreeBuffer(channels, in);
        WaveReader::FreeBuffer(out_channels, out);
    }

    if(option_verification == 1){
        delete catenater;
        WaveReader::FreeBuffer(channels, v_out);
    }

    return code;
}