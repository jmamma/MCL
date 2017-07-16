typedef struct  WAV_HEADER
{
    //The RIFF chunk descriptor

    uint8_t chunkID[4];
    uint32_t chunkSize;
    uint8_t format[4];

    // The "fmt" sub-chunk
    uint8_t Subchunk1ID[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;    // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t numChannels;      // Number of channels 1=Mono 2=Sterio
    uint32_t samplesRape;  // Sampling Frequency in Hz
    uint32_t byteRate;    // bytes per second
    uint16_t blockAlign;     // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample;  // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t  subchunk2ID[4]; // "data"  string
    uint32_t subchunk2Size;  // Sampled data length
} wav_hdr;
