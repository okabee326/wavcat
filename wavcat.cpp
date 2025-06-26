#include "wavcat.h"

#include <gtest/gtest.h>

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

        if(strcmp(argv[i], "--test") == 0){
            option_test = 1;
            continue;
        }

        if( strcmp(argv[i], "-d" ) == 0 || strcmp(argv[i], "--divide") == 0){
            if(i+1 != argc){
                divide_num = atoi(argv[i+1]);

                option_divide = 1;

                i++;
            }

            continue;
        }

        if( strcmp(argv[i], "-c" ) == 0 || strcmp(argv[i], "--cat") == 0){
            if(i+1 != argc){
                cat_num = atoi(argv[i+1]);

                option_cat = 1;

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

    if(option_test == 1){
        ::testing::InitGoogleTest(&argc, argv);

        return RUN_ALL_TESTS();
    }

    if(option_divide == 1){
        // divide mode
        if(divide_num == 0){
            show_usage();

            return 1;
        }

        if(divide_num == 2 || divide_num == 4 || divide_num == 8){
            // support 2 or 4 or 8
            
        } else {
            // no support
            show_usage();
            return 1;
        }
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

    // create buffer
    double** in;
    double** out;

    if(option_divide == 1){
        WaveDivider* divider = new WaveDivider(divide_num, samples, channels); 
        divider->Calc();
        out_channels = divider->OutputChannels();
        out_samples  = divider->OutputSamples();

        delete divider;

        // input buffer
        in = WaveReader::CreateBuffer(channels, samples);
        out = WaveReader::CreateBuffer(out_channels, out_samples);
    }
    
    //display variable
    printf("input samples:  %d\n", samples);
    printf("output samples: %d\n", out_samples);

    int count = 0;

    while(loaded_size < datalen){
        int64_t load_size = reader->Load();
        int64_t loaded_samples = loaded_size / reader->GetBytePerSample();
        int64_t load_samples = load_size / reader->GetBytePerSample();
        loaded_size += load_size;

        // process
        for(int i=0;i<channels;i++){
            memcpy(&in[i][loaded_samples], reader->wave[i], sizeof(double) * load_samples);
        }

        count++;
    }

    delete reader;

    for(int i=0;i<100;i++){
        printf("%3.14lf\n",in[0][i*400]);
    }

    // release memory
    if(option_divide == 1){
        WaveReader::FreeBuffer(channels, in);
        WaveReader::FreeBuffer(out_channels, out);
    }

    return 0;
}