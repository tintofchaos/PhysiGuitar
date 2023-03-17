#ifndef PICKUP_H_INCLUDED
#define PICKUP_H_INCLUDED

#include <math.h>
#include "DelayAllpass.h"

#define PICKUP_PRESET_CUSTOM 0x0
#define PICKUP_PRESET_TELE 0x1
#define PICKUP_PRESET_STRAT 0x2
#define PICKUP_PRESET_BASS 0x3

typedef struct {
    DelayAllpass delay;
    float B[3];
    float A[3];
    float inputs[2];
    float outputs[2];
    unsigned long sample_rate;
    float position;
} Pickup ;

int pickup_init(Pickup *pickup, unsigned long sample_rate) {
    pickup->sample_rate = sample_rate;
    return delayallpass_init(&pickup->delay, sample_rate);
}

void pickup_free(Pickup *pickup) {
    delayallpass_free(&pickup->delay);
}

void pickup_setposition(Pickup *pickup, float position) {
    pickup->position = position;
}

void pickup_setfrequency(Pickup *pickup, float frequency) {
    float delay = 1.f / frequency * pickup->position;

    delay *= (float) pickup->sample_rate;
    delayallpass_set(&pickup->delay, delay);
}

void pickup_setpickup(Pickup *pickup, float frequency, float Q, int preset, float tone) {
    float w0, a, cosw0, freq;
    switch (preset) {
        case PICKUP_PRESET_CUSTOM:
            w0 = 2.f * M_PI * frequency / (float) pickup->sample_rate;
            a = sinf(w0) / (2.f * Q);

            cosw0 = cosf(w0);

            pickup->B[0] = (1.f - cosw0 ) / 2.f;
            pickup->B[1] = pickup->B[0] * 2.f;
            pickup->B[2] = pickup->B[0];

            pickup->A[0] = 1.f + a;
            pickup->A[1] = -2.f * cosw0 * w0;
            pickup->A[2] = 1.f - a;

            break;

        case PICKUP_PRESET_TELE:
            freq = 1300.f + (2400.f * tone);
            w0 = 2.f * M_PI * freq / (float) pickup->sample_rate;
            a = sinf(w0) / (2.f * 3.7);

            cosw0 = cosf(w0);

            pickup->B[0] = (1.f - cosw0 ) / 2.f;
            pickup->B[1] = pickup->B[0] * 2.f;
            pickup->B[2] = pickup->B[0];

            pickup->A[0] = 1.f + a;
            pickup->A[1] = -2.f * cosw0 * w0;
            pickup->A[2] = 1.f - a;

            break;

        case PICKUP_PRESET_STRAT:
            freq = 1500.f + (2900.f * tone);

            w0 = 2.f * M_PI * freq / (float) pickup->sample_rate;
            a = sinf(w0) / (2.f * 6.3);

            cosw0 = cosf(w0);

            pickup->B[0] = (1.f - cosw0 ) / 2.f;
            pickup->B[1] = pickup->B[0] * 2.f;
            pickup->B[2] = pickup->B[0];

            pickup->A[0] = 1.f + a;
            pickup->A[1] = -2.f * cosw0 * w0;
            pickup->A[2] = 1.f - a;

            break;

        case PICKUP_PRESET_BASS:
            freq = 900.f + (2000.f * tone);

            w0 = 2.f * M_PI * freq / (float) pickup->sample_rate;
            a = sinf(w0) / (2.f * 5.4);

            cosw0 = cosf(w0);

            pickup->B[0] = (1.f - cosw0 ) / 2.f;
            pickup->B[1] = pickup->B[0] * 2.f;
            pickup->B[2] = pickup->B[0];

            pickup->A[0] = 1.f + a;
            pickup->A[1] = -2.f * cosw0 * w0;
            pickup->A[2] = 1.f - a;

            break;
    }
}

float pickup_process(Pickup *pickup, float sample) {
    delayallpass_write(&pickup->delay, sample);
    float delayed = -delayallpass_read(&pickup->delay) + sample;

    float filtered = delayed * pickup->B[0] + pickup->inputs[0] * pickup->B[1] + pickup->inputs[1] * pickup->B[2];
    filtered -= pickup->outputs[0] * pickup->A[1] + pickup->outputs[1] * pickup->A[2];
    filtered *= 1.f / pickup->A[0];

    pickup->inputs[1] = pickup->inputs[0];
    pickup->inputs[0] = delayed;

    pickup->outputs[1] = pickup->outputs[0];
    pickup->outputs[0] = filtered;

    return filtered;
}

#endif // PICKUP_H_INCLUDED
