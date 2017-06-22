#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <portaudio.h>
#include <kiss_fftr.h>
#include <audio_analyzer.hpp>
#include <shaderLoader.hpp>

float rawAudio[AudioAnalyzer::SAMPLE_SIZE]; // Declared here so the callback function can access it
static int paCallback(const void *inputBuffer,
                      void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData);

AudioAnalyzer::AudioAnalyzer() {
    spacing = SCREEN_WIDTH / ((float) SAMPLE_SIZE);
    bandSpacing = SCREEN_WIDTH / ((float) NUM_BANDS);

    // Setup VBOs
    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
    };
    glGenBuffers(1, &sVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    shader = loadShaders("resources/shaders/SimpleVertexShader.glsl", "resources/shaders/SimpleFragmentShader.glsl");

    // Initialize audio features
    PaError err = Pa_Initialize();
    if (paErrorOccured(err)) {
        return;
    } else {
        paInitSuccessful = true;
    }

    // Initialize kiss fft
    cfg = kiss_fftr_alloc(AudioAnalyzer::FFT_SIZE, false, 0,0);
    if (cfg == NULL) {
        fprintf(stderr, "Failed to initialize kiss fft");
        return;
    }

    // Precompute the hanning window
    computeHanningWindow();

    // Reset all buffers
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        rawAudio[i] = 0.0f;
        fft_in[i] = 0.0f;
        fft_out[i].i = 0.0f;
        fft_out[i].r = 0.0f;
    }
    for (int i = 0; i < NUM_BANDS / 2; i++) {
        processedAudio[i] = 0.0f;
    }
    for (int i = 0; i < NUM_BANDS; i++) {
        frequencyBands[i] = 0.0f;
    }

    printAudioDevices();

    int inDevNum = 0;
    int outDevNum = 0;

    if (MAC) {
        inDevNum = 4;
        outDevNum = 4;
    } else if (WIN) {
        inDevNum = 1;
        outDevNum = 3;
    } else if (LINUX) {
        inDevNum = 7;
        outDevNum = 7;
    }

    int inChan = 2;
    int outChan = 2;

    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.channelCount = inChan;
    inputParameters.device = inDevNum;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inDevNum)->defaultLowInputLatency ;

    memset(&outputParameters, 0, sizeof(outputParameters));
    outputParameters.channelCount = outChan;
    outputParameters.device = outDevNum;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outDevNum)->defaultLowOutputLatency ;
    err = Pa_OpenStream(
            &stream,
            &inputParameters,
            &outputParameters,
            SAMPLE_RATE,
            SAMPLE_SIZE,
            paNoFlag,
            paCallback,
            (void*) this
    );
    if (paErrorOccured(err)) return;

    err = Pa_StartStream(stream);
    if (paErrorOccured(err)) return;
}

void AudioAnalyzer::shutDown() {
    free(cfg);

    if (!paInitSuccessful) return;

    PaError err = Pa_StopStream(stream);
    if (paErrorOccured(err)) return;

    err = Pa_CloseStream(stream);
    if (paErrorOccured(err)) return;

    err = Pa_Terminate();
    if (paErrorOccured(err)) return;
}

void AudioAnalyzer::computeHanningWindow() {
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        // hanningWindow[i] = (float) (0.5f * (1.0f - cos((2 * M_PI * i) / (SAMPLE_SIZE - 1))));
        float val = (float) sin((M_PI * i) / (SAMPLE_SIZE - 1));
        hanningWindow[i] = val * val;
    }
}

static int paCallback(const void *inputBuffer,
                      void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
    {
    // Cast stream data to our structure
    float *in = (float*) inputBuffer;
    float *out = (float*) outputBuffer;

    for(unsigned int i = 0; i < framesPerBuffer; i++) {
        // Input buffer interleaves left and right channels
        // Mixing left and right channels into one mono channel
        rawAudio[i] = (in[i*2] + in[i*2+1]) / 2.0f;
    }

    return 0;
}

void AudioAnalyzer::update() {

    // Apply window function to raw audio data
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        rawAudio[i] = rawAudio[i] * hanningWindow[i];
        fft_in[i] = rawAudio[i];
    }

    // Perform the fft
    kiss_fftr(cfg, fft_in, fft_out);

    // To store the processed audio before it's converted to decibels
    float toBin[SAMPLE_SIZE / 2];

    // Process the fft output
    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        float real = fft_out[i].r;
        float imaginary = fft_out[i].i;
        float magnitude = sqrt(real * real + imaginary * imaginary);

        processedAudio[i] *= FREQUENCY_DAMPING;
        processedAudio[i] = max(20.0f * log10f(magnitude), processedAudio[i]);

        toBin[i] = magnitude;
    }

    // Bin similar frequencies into discrete bands
    float samplesPerBand = SAMPLE_SIZE / 2 / NUM_BANDS;
    for (int n = 0; n < NUM_BANDS; n++) {
        float value = 0.0f;

        for (int i = 0; i < samplesPerBand; i++) {
            value += toBin[i + (int) (n * samplesPerBand)];
        }

        frequencyBands[n] *= FREQUENCY_DAMPING;
        frequencyBands[n] = max(20.0f * log10f(value), frequencyBands[n]);
    }
}

void AudioAnalyzer::renderWaveform(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {1.0f, 0.0f, 0.0f, 0.0f};
    setColor(color);

    for (int i = 0; i < SAMPLE_SIZE; i++) {
        glm::mat4 translate = glm::translate(glm::vec3(i * spacing, rawAudio[i] * 350.0f + (SCREEN_HEIGHT * 0.5f), 0.0f));
        glm::mat4 scale = glm::scale(glm::vec3(spacing * 2.0f, spacing * 2.0f, 1.0f));
        drawSquare(transform * translate * scale, true);
    }
}

void AudioAnalyzer::renderSpectrum(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(color);

    for (int i = 0; i < SAMPLE_SIZE / 4; i++) {
        glm::mat4 translate = glm::translate(glm::vec3(i * spacing * 4, 0.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::vec3(spacing * 3, processedAudio[i] * 10.0f, 1.0f));
        drawSquare(transform * translate * scale, true);
    }
}

void AudioAnalyzer::renderFrequencyBands(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {0.0f, 1.0f, 0.0f, 0.0f};
    setColor(color);

    for (int i = 0; i < NUM_BANDS; i++) {
        glm::mat4 translate = glm::translate(glm::vec3(i * bandSpacing, SCREEN_HEIGHT, 0.0f));
        glm::mat4 scale = glm::scale(glm::vec3(bandSpacing * 0.75f, frequencyBands[i] * -10.0f, 1.0f));
        drawSquare(transform * translate * scale, true);
    }
}

void AudioAnalyzer::drawSquare(glm::mat4 transform, bool fill) {
    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, sVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    if (fill) {
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else {
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
//    glDisableVertexAttribArray(0);
}

void AudioAnalyzer::printAudioDevices() {

    // Get the number of devices
    int numDevices;
    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices);
        PaError err = numDevices;
        if (paErrorOccured(err)) return;
        return;
    }

    // Print out information for each device
    const PaDeviceInfo *deviceInfo;
    for(int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf("--- Device (%x)\n", i);
        printf("Name: %s\n", deviceInfo->name);
        printf("Input channels: %x\n", deviceInfo->maxInputChannels);
        printf("Output channels: %x\n", deviceInfo->maxOutputChannels);
    }

    printf("---\n");
}

bool AudioAnalyzer::paErrorOccured(PaError error) {
    if (error != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(error));
        return true;
    }

    return false;
}
