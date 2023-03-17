#include <math.h>
#include "SineWaveGen.h"
#define MAX_MODES_AMOUNT 64
#define DETUNE_AMOUNT 0.5 // in cents

typedef struct {
    SineWaveGen modes[MAX_MODES_AMOUNT];
    SineWaveGen coupling_modes[MAX_MODES_AMOUNT];
    float amplitudes[MAX_MODES_AMOUNT];
    float stiffness;
    float position;
    unsigned long sample_rate;
    float t;
    float damping;
    float muting;
    int updated;
    float frequency;
    float velocity;
    float dynamic_x_;
    float dynamic_y_;
    float A4;

} PluckedString ;

void string_init(PluckedString *string, unsigned long sample_rate, float A4) {
    for (int i = 0; i < MAX_MODES_AMOUNT; i++)
        sine_init(&string->modes[i], sample_rate);

    for (int i = 0; i < MAX_MODES_AMOUNT; i++)
        sine_init(&string->coupling_modes[i], sample_rate);

    string->sample_rate = sample_rate;
    string->A4 = A4;
    
    string->updated = 0;
    
    string->dynamic_x_ = 0;
    string->dynamic_y_ = 0;
    
    for (int i = 0; i < MAX_MODES_AMOUNT; i++)
        string->amplitudes[i] = 0;
}

void string_setstiffness(PluckedString *string, float stiffness) {
    string->stiffness = stiffness;
}

void string_setposition(PluckedString *string, float position) {
    string->position = position;
    string->updated = 0;
}

void string_setdamping(PluckedString *string, float damping) {
    string->damping = damping;
}

void string_setmuting(PluckedString *string, float muting) {
    string->muting = muting * 0.005;
}

// frequency of zero equals to same note

void string_setfrequency(PluckedString *string, float f0) {
    for (int i = 0; i < MAX_MODES_AMOUNT; i++) {
        float freq, n;
        n = (float) i + 1.f;
        freq = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );
        freq *= f0 * n;
        string->frequency = f0;
        if (freq < (float) string->sample_rate / 2.f)
            sine_setfrequency(&string->modes[i], freq);
    }

    float detune_note = 12.f * log2f(f0 / (string->A4 / 2.f) ) + 57.f + DETUNE_AMOUNT / 100.f;
    float detune_freq = powf(2.f, (detune_note - 69.f) / 12.f) * string->A4;

    for (int i = 0; i < MAX_MODES_AMOUNT; i++) {
        float freq, n;
        n = (float) i + 1.f;
        freq = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );
        freq *= detune_freq * n;
        if (freq < (float) string->sample_rate / 2.f)
            sine_setfrequency(&string->coupling_modes[i], freq);
    }
}

void string_noteon(PluckedString *string, float f0, float velocity) {
    float detune_f0, detune_note;
    if (f0 != 0) {
        detune_note = 12.f * log2f(f0 / (string->A4 / 2.f) ) + 57.f + DETUNE_AMOUNT / 100.f;
        detune_f0 = powf(2.f, (detune_note - 69.f) / 12.f) * string->A4;
    } else {
        detune_note = 12.f * log2f(string->frequency / (string->A4 / 2.f) ) + 57.f + DETUNE_AMOUNT / 100.f;
        detune_f0 = powf(2.f, (detune_note - 69.f) / 12.f) * string->A4;
    }

    for (int i = 0; i < MAX_MODES_AMOUNT; i++) {
        float freq, n;
        float detune_freq;

        // calculate overtone frequencies

        float freq_coeff = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );

        if (f0 != 0) {
            n = (float) i + 1.f;
            freq = freq_coeff * f0 * n;
            string->frequency = f0;
        } else {
            n = (float) i + 1.f;
            freq = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );
            freq = freq_coeff * string->frequency * n;
        }
        
        sine_setfrequency(&string->modes[i], freq);

        if (!string->updated) {
            string->amplitudes[i] = 2.f;
            string->amplitudes[i] /= (M_PI * M_PI) * (n * n) * string->position * (1.f - string->position);
            string->amplitudes[i] *= sinf( (M_PI * n * string->position) / 1.f);
        }

        // calculate detuned overtone frequencies

        if (f0 != 0) {
            n = (float) i + 1.f;
            detune_freq = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );
            detune_freq *= detune_f0 * n;
        } else {
            n = (float) i + 1.f;
            detune_freq = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );
            detune_freq *= detune_f0 * n;
        }
        
        sine_setfrequency(&string->coupling_modes[i], detune_freq);
    }

    string->updated = 1;
    string->t = 0;
    string->velocity = velocity;
}

float string_process(PluckedString *string) {

    float sum = 0;
    
    float detune_note = 12.f * log2f(string->frequency / (string->A4 / 2.f) ) + 57.f + DETUNE_AMOUNT / 100.f;
    float detune_f0 = powf(2.f, (detune_note - 69.f) / 12.f) * string->A4;

    for (int i = 0; i < MAX_MODES_AMOUNT; i++) {
        float n = (float) i + 1.f;

        float freq = sqrtf(1.f + (string->stiffness * string->stiffness) * (n * n) );
        float detune_freq = freq;
        
        detune_freq *= n * detune_f0;
        freq *= string->frequency * n;

        if (freq >= (float) string->sample_rate / 2.f)
            break;
            
        float decay_time = string->damping + string->muting * freq * M_PI * 2.f;

        sum += sine_process(&string->modes[i]) * string->amplitudes[i] * expf(-string->t * decay_time) / (float) MAX_MODES_AMOUNT;

        if (detune_freq >= (float) string->sample_rate / 2.f)
            break;

        sum += sine_process(&string->coupling_modes[i]) * string->amplitudes[i] * expf(-string->t * decay_time) / (float) MAX_MODES_AMOUNT;
    }
    
    string->t += 1.f / (float) string->sample_rate;

    // velocity level filter

    float w = M_PI * string->frequency / (float) string->sample_rate;

    float temp = w / (1.f + w) * (sum + string->dynamic_x_) + (1.f - w) / (1.f + w) * string->dynamic_y_;

    string->dynamic_x_ = sum;
    string->dynamic_y_ = temp;

    float output = (powf(string->velocity, 4.f/3.f) * sum) + (1.f - string->velocity) * temp;

    return sum;
}
