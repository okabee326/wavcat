#include "wavcat.h"
#include <gtest/gtest.h>

TEST(TestWaveDivider, TestWaveDividerCalc)
{
    WaveDivider *divider = new WaveDivider(2, 2048, 2);
    divider->Calc();

    int channels, samples;
    channels = divider->OutputChannels();
    samples = divider->OutputSamples();

    EXPECT_EQ(channels, 4);
    EXPECT_EQ(samples, 1024);

    // test2
    divider->SetParam(2, 1025, 2);
    divider->Calc();
    channels = divider->OutputChannels();
    samples = divider->OutputSamples();

    EXPECT_EQ(channels, 4);
    EXPECT_EQ(samples, 513);

    // test3
    divider->SetParam(4, 1027, 2);
    divider->Calc();
    channels = divider->OutputChannels();
    samples = divider->OutputSamples();

    EXPECT_EQ(channels, 8);
    EXPECT_EQ(samples, 257);

    // finish
    delete divider;
}

TEST(TestWaveDivider, TestWaveCatenaterCalc)
{
    WaveCatenater *wavecat = new WaveCatenater(2, 512, 4);

    int samples, channels;

    wavecat->Calc();
    EXPECT_EQ(wavecat->OutputChannels(), 2);
    EXPECT_EQ(wavecat->OutputSamples(), 1024);

    // test2
    wavecat->SetParam(2, 513, 4);
    wavecat->Calc();
    EXPECT_EQ(wavecat->OutputChannels(), 2);
    EXPECT_EQ(wavecat->OutputSamples(), 1026);

    // test3
    wavecat->SetParam(4, 257, 8);
    wavecat->Calc();
    EXPECT_EQ(wavecat->OutputChannels(), 2);
    EXPECT_EQ(wavecat->OutputSamples(), 1028);

    // finish
    delete wavecat;
}

TEST(TestWaveDivider, TestWaveDivide)
{
    double **in;
    double **out;
    double **out2;
    int channels = 2;
    int samples = 513;
    int divide_num = 4;

    WaveDivider *divider = new WaveDivider(divide_num, samples, channels);
    divider->Calc();

    WaveCatenater *cater = new WaveCatenater(divide_num, 
                                             divider->OutputSamples(), 
                                             divider->OutputChannels());
    cater->Calc();

    // prepare data

    in = (double **)malloc(sizeof(double *) * channels);
    for (int i = 0; i < channels; i++)
    {
        in[i] = (double *)malloc(sizeof(double) * samples);
        memset(in[i], 0, sizeof(double) * samples);
    }

    // set input data
    for(int i=0;i<samples;i++){
        in[0][i] = (double)i;
    }

    //printf("point1\n");

    out = (double **)malloc(sizeof(double *) * channels * divide_num);
    for (int i = 0; i < divider->OutputChannels(); i++)
    {
        out[i] = (double *)malloc(sizeof(double) * divider->OutputSamples());
        memset(out[i], 0, sizeof(double) * divider->OutputSamples());
    }

    out2 = (double**)malloc( sizeof(double*) * cater->OutputChannels() );
    for(int i=0;i<cater->OutputSamples();i++){
        out2[i] = (double*)malloc(sizeof(double) * cater->OutputSamples() );
        memset(out2[i], 0, sizeof(double) * cater->OutputSamples() );
    }

    //printf("point2\n");

    // test code
    // divide
    divider->Divide(in, out);

    EXPECT_EQ(out[0][0], 0);
    EXPECT_EQ(out[0][128], 128);
    EXPECT_EQ(out[1][0], 129);
    EXPECT_EQ(out[2][0], 258);

    // catenate
    cater->Catenate(out, out2);

    EXPECT_EQ(out2[0][129], 129);
    EXPECT_EQ(out2[0][512], 512);

    /*
    for(int i=0;i<200;i++){
        printf("%d\n", (int)out2[0][i]);
    }
    */

    // release memory
    /*
    for (int i = 0; i < channels; i++)
    {
        free(in[i]);
    }
    free(in);

    for (int i = 0; i < divider->OutputChannels(); i++)
    {
        free(out[i]);
    }
    free(out);

    for(int i=0;i< cater->OutputChannels();i++){
        free(out2[i]);
    }
    free(out2);
    */

    delete divider;
    delete cater;
}