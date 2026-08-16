#include "vu-meter.h"

#include <math.h>
#include <stdio.h>

static const int lights_per_dB = 3;
static const int light_count = 10;
static char stars[10 + 1]; /* must match light_count */

void vu_meter_init() {
    int i;
    for(i = 0; i < light_count; i++) {
        stars[i] = '*';
    }
    stars[light_count] = 0;
}

void vu_meter_on_sample(int channels, int count, float *buffer) {
    (void)channels;
    float sum = 0.0f;
    float volume = 0.0f;

    if( count <= 0 ) {
        return;
    }

    for(int i = 0; i < count; i++) {
        float sample = buffer[i];
        sum += sample * sample;
    }

    /* RMS level in dBFS */
    volume = 20.0f * log10f(sqrtf(sum / (float)count));

    const float origVolume = volume;

    /* Map volume onto 0..light_count bars (lights_per_dB steps) */
    volume += (float)(light_count * lights_per_dB);
    volume = fmaxf(0.0f, volume);
    volume /= (float)lights_per_dB;
    int v = (int)fminf(volume, (float)light_count);
    printf("%6.2f %d %s\n", origVolume, v, stars + (light_count - v));
}

// vim: set ts=8 sw=4 sts=4 expandtab:
