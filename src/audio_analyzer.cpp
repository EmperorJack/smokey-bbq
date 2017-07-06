#include <iostream>
#include <vector>
#include <main.hpp>
#include <opengl.hpp>
#include <portaudio.h>
#include <kiss_fftr.h>
#include <audio_analyzer.hpp>
#include <shaderLoader.hpp>

float rawAudio[AudioAnalyzer::SAMPLE_SIZE]; // Declared here so the callback function can access it
float hanningWindow[AudioAnalyzer::SAMPLE_SIZE];
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
    float waveformVertices[SAMPLE_SIZE * 2] = {};

    glGenBuffers(1, &waveformVBO);
    glBindBuffer(GL_ARRAY_BUFFER, waveformVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(waveformVertices), waveformVertices, GL_STATIC_DRAW);

    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
    };
    glGenBuffers(1, &squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
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
    cfg = kiss_fftr_alloc(AudioAnalyzer::SAMPLE_SIZE, false, 0,0);
    if (cfg == NULL) {
        fprintf(stderr, "Failed to initialize kiss fft");
        return;
    }

    // Precompute the hanning window
    computeHanningWindow();

    resetBuffers();
    setDefaultVariables();
    setDefaultToggles();

    // Compute log mapping
    computeLogMapping();

    printAudioDevices();

    int inDevNum = 0;
    int outDevNum = 0;

    #ifdef __APPLE__
    inDevNum = 0;
    outDevNum = 2;
    #endif

    #ifdef __MINGW32__
    inDevNum = 1;
    outDevNum = 3;
    #endif

    #ifdef __linux__
    inDevNum = 7;
    outDevNum = 7;
    #endif

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

void AudioAnalyzer::resetBuffers() {
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        rawAudio[i] = 0.0f;
        fft_in[i] = 0.0f;
        fft_out[i].i = 0.0f;
        fft_out[i].r = 0.0f;
    }
    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        processedAudio[i] = 0.0f;
    }
    for (int i = 0; i < NUM_BANDS; i++) {
        frequencyBands[i] = 0.0f;
    }
}

void AudioAnalyzer::setDefaultVariables() {
    FREQUENCY_DAMPING = 0.85f;
}

void AudioAnalyzer::setDefaultToggles() {
    displayWaveform = false;
    displaySpectrum = false;
    displayFrequencyBands = false;
    updateAnalyzer = true;
    logScaleBands = true;
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

void AudioAnalyzer::computeLogMapping() {
    float bandThresholds[NUM_BANDS];

    for (int i = 0; i < NUM_BANDS; i++) {
        float pct = i / (float) NUM_BANDS;
        // float val = log10f(i) / log10f(NUM_BANDS);
        // std::cout << "band (" << i << ") = " << val << std::endl;
        bandThresholds[i] = pct;
    }

    float sampleThresholds[SAMPLE_SIZE / 2];

    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        // float pct = i / (float) (SAMPLE_SIZE / 2);
        float val = log10f(i) / log10f(SAMPLE_SIZE / 2);
        // std::cout << "sample (" << i << ") = " << pct << std::endl;
        sampleThresholds[i] = val;
    }

    std::vector<int> mappings[NUM_BANDS];

    for (int i = 0; i < NUM_BANDS; i++) {
        float minValue = bandThresholds[i];
        float maxValue = (i+1 == NUM_BANDS ? 1.0f : bandThresholds[i+1]);

        for (int j = 0; j < SAMPLE_SIZE / 2; j++) {
            if (minValue <= sampleThresholds[j] && sampleThresholds[j] < maxValue) {
                mappings[i].push_back(j);
                mapping[j] = i;
            }
        }
    }

    //    for (int i = 0; i < NUM_BANDS; i++) {
    //        std::cout << "(" << i << ") : [";
    //        for (int j : mappings[i]) {
    //            std::cout << " " << j << ", ";
    //        }
    //        std::cout << "]" << std::endl;
    //    }
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
        rawAudio[i] = (in[i*2] + in[i*2+1]) * 0.5f * hanningWindow[i];
    }

    return 0;
}

void AudioAnalyzer::update() {
    if (!updateAnalyzer) return;

    // Apply window function to raw audio data
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        fft_in[i] = rawAudio[i];
    }

    // Perform the fft
    kiss_fftr(cfg, fft_in, fft_out);

    // To store the processed audio before it's converted to decibels
    float toBin[NUM_BANDS] = {};
    // float toBin[SAMPLE_SIZE / 2];

    // Process the fft output
    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        float real = fft_out[i].r;
        float imaginary = fft_out[i].i;
        float magnitude = sqrt(real * real + imaginary * imaginary);

        processedAudio[i] *= FREQUENCY_DAMPING;
        processedAudio[i] = max(20.0f * log10f(magnitude), processedAudio[i]);

        toBin[mapping[i]] += magnitude * 0.15f;
        // toBin[i] = magnitude;
    }

    // Bin similar frequencies into discrete bands
//    float samplesPerBand = SAMPLE_SIZE / NUM_BANDS / 2.0f;
    for (int n = 0; n < NUM_BANDS; n++) {
//        float value = 0.0f;
//
//        for (int i = 0; i < samplesPerBand; i++) {
//            value += toBin[i + (int) (n * samplesPerBand)];
//        }

        frequencyBands[n] *= FREQUENCY_DAMPING;
        frequencyBands[n] = max(20.0f * log10f(toBin[n]), frequencyBands[n]);
        // frequencyBands[n] = max(20.0f * log10f(value), frequencyBands[n]);
    }
}

float AudioAnalyzer::getFrequencyBand(int i) {
    return frequencyBands[i];
}

void AudioAnalyzer::render(glm::mat4 transform) {
    if (displayWaveform) renderWaveform(transform);
    if (displaySpectrum) {
        if (logScaleBands) renderLogSpectrum(transform);
        else renderLinearSpectrum(transform);
    }
    if (displayFrequencyBands) renderFrequencyBands(transform);
}

void AudioAnalyzer::renderWaveform(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {1.0f, 0.0f, 0.0f, 0.0f};
    setColor(shader, color);

    float waveformVertices[SAMPLE_SIZE * 2];

    for (int i = 0; i < SAMPLE_SIZE; i++) {
        waveformVertices[i*2] = i * spacing;
        waveformVertices[i*2+1] = rawAudio[i] * 350.0f + (SCREEN_HEIGHT * 0.5f);
    }

    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, waveformVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0.0f, sizeof(waveformVertices), waveformVertices);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_LINE_STRIP, 0, SAMPLE_SIZE);
    glDisableVertexAttribArray(0);
}

void AudioAnalyzer::renderLinearSpectrum(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(shader, color);

    for (int i = 0; i < SAMPLE_SIZE / 4; i++) {
        glm::mat4 translate = glm::translate(glm::vec3(i * spacing * 4, 0.0f, 0.0f));
        glm::mat4 scale = glm::scale(glm::vec3(spacing * 3, processedAudio[i] * 10.0f, 1.0f));
        drawSquare(transform * translate * scale, true);
    }
}

void AudioAnalyzer::renderLogSpectrum(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(shader, color);

    float logOver = log10f(SAMPLE_SIZE / 2);

    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        float pct = i / (float) (SAMPLE_SIZE / 2);

        float x = pct * SCREEN_WIDTH;
        // x = log10f(i) / logOver * SCREEN_WIDTH;

        float fIndex = (powf(10, pct * logOver));
        int minIndex = (int) max(fIndex, 0.0f);
        int maxIndex = (int) min(fIndex + 1.0f, SAMPLE_SIZE / 2 - 1.0f);

        float value = lerp(processedAudio[minIndex], processedAudio[maxIndex], fIndex - minIndex);
        // value = processedAudio[i];

        glm::mat4 translate = glm::translate(glm::vec3(x, 0.0f, 0.0f)); // i * spacing * 4
        glm::mat4 scale = glm::scale(glm::vec3(spacing * 1.5f, value * 10.0f, 1.0f));
        drawSquare(transform * translate * scale, true);
    }
}

void AudioAnalyzer::renderFrequencyBands(glm::mat4 transform) {
    glUseProgram(shader);

    float color[] = {0.0f, 1.0f, 0.0f, 0.0f};
    setColor(shader, color);

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
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
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
    glDisableVertexAttribArray(0);
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
