#include <iostream>
#include <opengl.hpp>
#include <portaudio.h>
#include <audio_analyzer.hpp>
#include <shaderLoader.hpp>
#include <string.h>

// GL variables
GLuint sVBO;
GLuint shader;

// Instance variables
float screenWidth, screenHeight;
float spacing;

// Port audio variables
bool paInitSuccessful = false;
PaStream *stream;
PaStreamParameters outputParameters;
PaStreamParameters inputParameters;

// Audio data variables
float rawAudio[AudioAnalyzer::SAMPLE_SIZE];

bool paErrorOccured(PaError error) {
    if (error != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(error));
        return true;
    }

    return false;
}

static int paCallback(const void *inputBuffer,
                      void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData);

AudioAnalyzer::AudioAnalyzer(float _screenWidth, float _screenHeight) {
    screenWidth = _screenWidth;
    screenHeight = _screenHeight;

    spacing = _screenWidth / ((float) SAMPLE_SIZE);

    // Setup VBOs
    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, spacing,
            spacing, spacing,
            spacing, 0.0f,
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

    int inDevNum = 1;
    int inChan = 2;
    int outDevNum = 3;
    int outChan = 2;

    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.channelCount = inChan;
    inputParameters.device = inDevNum;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inDevNum)->defaultLowInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    memset(&outputParameters, 0, sizeof(outputParameters));
    outputParameters.channelCount = outChan;
    outputParameters.device = outDevNum;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outDevNum)->defaultLowOutputLatency ;
    outputParameters.hostApiSpecificStreamInfo = NULL;
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
    if (!paInitSuccessful) return;

    PaError err = Pa_StopStream( stream );
    if (paErrorOccured(err)) return;

    err = Pa_CloseStream( stream );
    if (paErrorOccured(err)) return;

    err = Pa_Terminate();
    if (paErrorOccured(err)) return;
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
        deviceInfo = Pa_GetDeviceInfo( i );
        printf("--- Device (%x)\n", i);
        printf("Name: %s\n", deviceInfo->name);
        printf("Input channels: %x\n", deviceInfo->maxInputChannels);
        printf("Output channels: %x\n", deviceInfo->maxOutputChannels);
    }

    printf("---\n");
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
        rawAudio[i] = *in++;
    }

    return 0;
}

void AudioAnalyzer::render(glm::mat4 transform) {
    glUseProgram(shader);

    for (int i = 0; i < SAMPLE_SIZE; i++) {
        float color[] = {1.0f, 0.0f, 0.0f, 0.0f};
        setColor(color);

        glm::mat4 translate = glm::translate(glm::vec3(i * spacing, rawAudio[i] * 250.0f + (screenHeight * 0.5f), 0.0f));
        glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
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
    glDisableVertexAttribArray(0);
}
