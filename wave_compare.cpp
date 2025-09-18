#include "wave_compare.h"

WaveCompare::WaveCompare(char* file1, char* file2, char* diff_file){
    //
    data_diff = NULL;
    channels = 0;
    blocksize = 32768;

    reader1 = NULL;
    reader2 = NULL;
    writer = NULL;

    Load(file1, file2, diff_file);
}

WaveCompare::~WaveCompare(){
    //
    if(data_diff != NULL && channels > 0){
        WaveReader::FreeBuffer(channels, data_diff);
    }

    if(reader1 != NULL){
        delete reader1;
    }

    if(reader2 != NULL){
        delete reader2;
    }
}

void WaveCompare::Load(char* file1, char* file2, char* diff_file){
    int64_t samples2;
    int64_t datalen2;

    reader1 = new WaveReader(blocksize);
    reader2 = new WaveReader(blocksize);

    // read header
    reader1->Init(file1);
    reader2->Init(file2);

    // get buffer size
    int64_t buffersize = reader1->GetBufferSize();
    channels = reader1->GetChannels();
    int64_t ch2 = reader2->GetChannels();
    if(ch2 < channels){
        channels = ch2;
    }

    samples = reader1->GetSamples();
    samples2 = reader2->GetSamples();

    if(samples2 < samples){
        samples = samples2;
    }

    datalen = reader1->GetDataLen();
    datalen2 = reader2->GetDataLen();

    if(datalen2 < datalen){
        datalen = datalen2;
    }

    // create write buffer
    data_diff = WaveReader::CreateBuffer(channels, samples);

}

int WaveCompare::Compare(){
    if(data_diff == NULL){
        return 1;
    }

    int64_t loaded_size = 0;

    int byte_per_sample = reader1->GetBytePerSample();

    // load all wave data
    while (loaded_size < datalen){
        // load wave data
        int64_t load_size = reader1->Load();
        reader2->Load();

        printf("load_size: %d\n", load_size);

        // calculate diff
        for(int c=0;c<channels;c++){
            for(int i=0;i<blocksize;i++){
                data_diff[c][i] = reader1->wave[c][i] - reader2->wave[c][i];
            }
        }

        for(int j=0;j<blocksize;j++){
            printf("%3.14f\n", data_diff[0][j]);
        }

        loaded_size += load_size;

        printf("%d / %d\n", loaded_size, datalen);

    }

    return 0;
}