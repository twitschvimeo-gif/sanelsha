#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <portaudio.h>
#include "vu-meter.h"

void usage(char* argv0) {
    fprintf(stderr, "%s -l\n", argv0);
    fprintf(stderr, "%s -r -i <input device index>\n", argv0);
}

void init_pa_or_die() {
    /* -- initialize PortAudio -- */
    PaError err = Pa_Initialize();
    if( err != paNoError ) {
        fprintf(stderr, "Unable to initialize PortAudio: %s\n", Pa_GetErrorText(err));
        exit(1);
    }
}

void terminate_pa() {
    Pa_Terminate();
}

int list() {
    init_pa_or_die();

    PaDeviceIndex count = Pa_GetDeviceCount();
    printf("Device count: %d\n", count);
    printf("\n");

    for(PaDeviceIndex i = 0; i < count; i++) {
        const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
        if( info == NULL ) {
            continue;
        }

        printf("#%d: %s\n", i, info->name);
        printf(" Default sample rate: %g\n", info->defaultSampleRate);
    }
    printf("\n");

    printf("Defaults\n");
    printf(" Input device: %d\n", Pa_GetDefaultInputDevice());
    printf(" Output device: %d\n", Pa_GetDefaultOutputDevice());

    terminate_pa();
    return 0;
}

int run(PaDeviceIndex inIndex) {
    init_pa_or_die();

    PaError err;

    PaStreamParameters inputParameters;
    PaStream *stream;
    const int numChannels = 1;
    const double sampleRate = 44100;
    const unsigned long framesPerBuffer = 100;
    const PaSampleFormat sampleType = paFloat32;

    float sampleBlock[framesPerBuffer];

    /* -- setup input -- */
    memset( &inputParameters, 0, sizeof( inputParameters ) );
    if( inIndex < 0 ) {
        inIndex = Pa_GetDefaultInputDevice();
    }
    if( inIndex == paNoDevice ) {
        fprintf(stderr, "No input device available\n");
        terminate_pa();
        return 1;
    }
    inputParameters.device = inIndex;
    inputParameters.channelCount = numChannels;
    inputParameters.sampleFormat = sampleType;
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( inputParameters.device );
    if( deviceInfo == NULL ) {
        fprintf(stderr, "Invalid input device index: %d\n", inputParameters.device);
        terminate_pa();
        return 1;
    }
    inputParameters.suggestedLatency = deviceInfo->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    printf("Input\n");
    printf("  device: %s (%d)\n", deviceInfo->name, inputParameters.device);
    printf("  # channels: %d\n", inputParameters.channelCount);

    /* -- setup stream -- */
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,
        sampleRate,
        framesPerBuffer,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
        NULL, /* no callback, use blocking API */
        NULL ); /* no callback, so no callback userData */
    if( err != paNoError ) {
        fprintf(stderr, "Unable to open stream: %s\n", Pa_GetErrorText(err));
        terminate_pa();
        return 1;
    }

    /* -- start stream -- */
    err = Pa_StartStream( stream );
    if( err != paNoError ) {
        fprintf(stderr, "Unable to start stream: %s\n", Pa_GetErrorText(err));
        Pa_CloseStream( stream );
        terminate_pa();
        return 1;
    }
    printf("Wire on. Will run one minute.\n"); fflush(stdout);

    printf("framesPerBuffer=%lu\n", framesPerBuffer);

    /* -- Read audio and feed the VU meter -- */
    for( int i=0; i<(int)((60*sampleRate)/framesPerBuffer); ++i )
    {
        err = Pa_ReadStream( stream, sampleBlock, framesPerBuffer );
        if( err != paNoError ) {
            fprintf(stderr, "Error reading stream: %s\n", Pa_GetErrorText(err));
            break;
        }

        vu_meter_on_sample(inputParameters.channelCount, (int)framesPerBuffer, sampleBlock);
    }
    /* -- Now we stop the stream -- */
    err = Pa_StopStream( stream );
    if( err != paNoError ) {
        fprintf(stderr, "Error stopping stream: %s\n", Pa_GetErrorText(err));
    }

    /* -- don't forget to cleanup! -- */
    err = Pa_CloseStream( stream );
    if( err != paNoError ) {
        fprintf(stderr, "Error closing stream: %s\n", Pa_GetErrorText(err));
    }

    terminate_pa();
    return 0;
}

enum Mode {
    MODE_RUN,
    MODE_LIST,
    MODE_HELP
};

int main(int argc, char* argv[]) {
    PaDeviceIndex inIndex = -1;

    enum Mode mode = MODE_HELP;
    int c;
    while ( (c = getopt(argc, argv, "lhri:")) != -1) {
        switch(c) {
            case 'l':
                mode = MODE_LIST;
                break;
            case 'r':
                mode = MODE_RUN;
                break;
            case 'h':
                mode = MODE_HELP;
                break;
            case 'i':
                inIndex = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Unknown argument\n");
                usage(argv[0]);
                return 1;
        }
    }

    vu_meter_init();

    switch(mode) {
        case MODE_LIST:
            return list();
        case MODE_RUN:
            return run(inIndex);
        case MODE_HELP:
        default:
            usage(argv[0]);
            return 0;
    }
}

// vim: set ts=8 sw=4 sts=4 expandtab:
