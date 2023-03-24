#include <math.h>

#define WAVETABLE_SIZE 2048

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI

typedef struct {
    float wavetable[WAVETABLE_SIZE];
    unsigned long sample_rate;
    float freq;
    float wavetable_pos;
} SineWaveGen ;

void sine_init(SineWaveGen *sine, unsigned long sample_rate) {
    sine->sample_rate = sample_rate;
    sine->wavetable_pos = 0;

    float freq = (float) sample_rate / (float) WAVETABLE_SIZE;

    for (int i = 0; i < WAVETABLE_SIZE; i++)
        sine->wavetable[i] = sinf(2.f * M_PI * freq * ( (float) i / (float) sample_rate) );

}

void sine_setfrequency(SineWaveGen *sine, float freq) {
    sine->freq = freq;
}

void sine_reset(SineWaveGen *sine) {
    sine->wavetable_pos = 0;
}

float sine_process(SineWaveGen *sine) {
    float freq = (float) sine->sample_rate / (float) WAVETABLE_SIZE;

    float low = floorf(sine->wavetable_pos);
    float high = ceilf(sine->wavetable_pos);
    float output = sine->wavetable[(int)sine->wavetable_pos];
    
    if (low != high)
        output = (sine->wavetable_pos - low) * sine->wavetable[(int)high] + (high - sine->wavetable_pos) * sine->wavetable[(int)low];
        
    sine->wavetable_pos += sine->freq / freq;

    if (sine->wavetable_pos > (float) WAVETABLE_SIZE - 1.f)
        sine->wavetable_pos -= (float) WAVETABLE_SIZE - 1.f;

    return output;
}
