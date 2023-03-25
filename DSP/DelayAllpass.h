#include <stdlib.h>
#include <math.h>

typedef struct {
    float *buffer;
    int max_delay;
    int read_pos;
    int write_pos;

    float frac_delay;

    float prev_input;
    float prev_output;
} DelayAllpass;

int delayallpass_init(DelayAllpass *delay, int max_delay) {
    delay->read_pos = 0;
    delay->write_pos = 0;
    delay->max_delay = max_delay;
    delay->buffer = (float*) malloc(sizeof(float) * max_delay );

    delay->prev_input = 0;
    delay->prev_output = 0;

    delay->frac_delay = 0;

    return delay->buffer != NULL;
}

void delayallpass_set(DelayAllpass *delay, float len) {
    int int_delay = (int) floorf(len);
    delay->read_pos = delay->write_pos - int_delay;
    while (delay->read_pos < 0)
        delay->read_pos += delay->max_delay;

    delay->frac_delay = len - floorf(len);
}

float delayallpass_read(DelayAllpass *delay) {
    float val = delay->buffer[delay->read_pos++];

    float coeff = (1 - delay->frac_delay) / (1 + delay->frac_delay);

    float output = coeff * val + delay->prev_input - coeff * delay->prev_output;
    delay->prev_input = val;
    delay->prev_output = output;

    if (delay->read_pos >= delay->max_delay)
        delay->read_pos = 0;

    return output;
}

void delayallpass_write(DelayAllpass *delay, float input) {
    delay->buffer[delay->write_pos++] = input;

    if (delay->write_pos >= delay->max_delay)
        delay->write_pos = 0;
}

void delayallpass_free(DelayAllpass *delay) {
    free(delay->buffer);
}
