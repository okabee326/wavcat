#include "wave_reader.h"
#include <gtest/gtest.h>

TEST( wave_reader,  wave_load ){
    int blocksize = 32768;
    int64_t loaded_size = 0;

    WaveReader *reader = new WaveReader(blocksize);

    EXPECT_EQ(1,1);

    delete reader;
}
