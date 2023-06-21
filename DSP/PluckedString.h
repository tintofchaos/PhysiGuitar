#include <math.h>
#define MAX_MODES_AMOUNT 32
#define DETUNE_AMOUNT 0.5 // in cents

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI

// youngs modulus of strings
#define STELL_STIFFNESS 0.1570795
#define NYLON_STIFFNESS 0.0043196

typedef struct {
    float b[MAX_MODES_AMOUNT];
    float a1[MAX_MODES_AMOUNT];
    float a2[MAX_MODES_AMOUNT];
    
    float x_[MAX_MODES_AMOUNT];
    float y_[MAX_MODES_AMOUNT];
    float y__[MAX_MODES_AMOUNT];

    float amplitudes[MAX_MODES_AMOUNT];
    
    float material;
    float position;
    unsigned long sample_rate;
    
    float decay;
    float damping;
    float width;
    
    short harmonics;
    
    int updated;
    
    float frequency;
    float A4;
    
    float excitation;
    
    int noteoff;
} PluckedString ;

void string_init(PluckedString *string, unsigned long sample_rate, float A4) {
    string->sample_rate = sample_rate;
    string->A4 = A4;
    
    string->updated = 0;

    for (int i = 0; i < MAX_MODES_AMOUNT; i++) {
        string->amplitudes[i] = 0;
        string->x_[i] = 0;
        string->y_[i] = 0;
        string->y__[i] = 0;
    }
}

void string_setmaterial(PluckedString *string, float material) {
    string->material = NYLON_STIFFNESS + (STELL_STIFFNESS - NYLON_STIFFNESS) * material;
    string->updated = 0;
}

void string_setposition(PluckedString *string, float position) {
    string->position = position;
    string->updated = 0;
}

void string_setdecay(PluckedString *string, float decay) {
    string->decay = decay * 8.f;
    string->updated = 0;
}

void string_setdamping(PluckedString *string, float damping) {
    string->damping = damping * 0.005;
    string->updated = 0;
}

void string_setfrequency(PluckedString *string, float f0) {
    string->frequency = f0;
    string->updated = 0;
}

void string_setharmonics(PluckedString *string, short harmonics) {
    string->harmonics = harmonics;
}

void string_update(PluckedString *string) {
    if (!string->updated) {
        float freq, n;
        float freq_coeff;
        float decay;

        for (int i = 0; i < MAX_MODES_AMOUNT; i++) {
            // calculate overtone frequencies
            if (freq >= (float) string->sample_rate / 2.f)
                break;
                
            float tension = string->frequency * string->frequency * 4.f;
            float stiffness = 0.f;

            stiffness = string->material / tension * 9.86960440109;

            n = (float) i + 1.f;
            freq_coeff = sqrtf(1.f + (stiffness * stiffness) * (n * n) );
            freq = sqrtf(1.f + (stiffness * stiffness) * (n * n) );
            freq = freq_coeff * string->frequency * n;

            string->amplitudes[i] = 2.f;
            string->amplitudes[i] /= (M_PI * M_PI) * (n * n) * string->position * (1.f - string->position);
            string->amplitudes[i] *= sinf( (M_PI * n * string->position) / 1.f);
            
            if (n >= (2.f / (M_PI * string->width) ) )
                string->amplitudes[i] *= (2.f / (M_PI * string->width * n) );

            decay = (string->decay + string->damping * freq * 2.f * M_PI);
            
            float radius = expf(-decay / (float) string->sample_rate);
            
            string->b[i] = string->amplitudes[i] * radius * sinf(2.f * M_PI * (freq / (float) string->sample_rate) );
            string->a1[i] = -2.f * radius * cosf(2.f * M_PI * (freq / (float) string->sample_rate) );
            string->a2[i] = radius * radius;
            
        }

        string->updated = 1;
    }
}

void string_noteon(PluckedString *string) {
    string->excitation = 1.f;
    string->noteoff = 0;
}

void string_setwidth(PluckedString *string, float width) {
    string->width = width * 0.1;
}

void string_noteoff(PluckedString *string) {
    string->noteoff = 1;
}

float string_process(PluckedString *string) {
    float sum = 0.f;
    float output = 0.f;
    
    for (int i = 0; i < MAX_MODES_AMOUNT; i++) {    
        output = (string->excitation * string->b[i] - string->a1[i] * string->y_[i] - string->a2[i] * string->y__[i]);
        string->y__[i] = string->y_[i];
        string->y_[i] = output;
        string->x_[i] = string->excitation;
        
        float harmonic_on = 1.f;
        
        if (string->harmonics) {
            if (((i + 1) % 2) != 0)
                harmonic_on = 0.f;
        }
        
        sum += output * harmonic_on;
    }
    
    if (string->excitation == 1.f)
        string->excitation = 0.f;
            
    return sum * 0.1;
}
