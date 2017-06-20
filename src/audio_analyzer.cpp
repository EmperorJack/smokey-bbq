#include <iostream>
#include <opengl.hpp>
#include <portaudio.h>
#include <audio_analyzer.hpp>
#include <shaderLoader.hpp>

#define MUS_PATH "resources/shaders/kick.wav"

GLuint sVBO;
GLuint shader;

bool successInit = false;

#define SAMPLE_RATE (44100)

typedef struct
{
    float left_phase;
    float right_phase;
} paTestData;

static paTestData data;
PaStream *stream;

/* This routine will be called by the PortAudio engine when audio is needed.
   It may called at interrupt level on some machines so don't do anything
   that could mess up the system like calling malloc() or free().
*/
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData );

AudioAnalyzer::AudioAnalyzer() {
    float spacing = 800 / 128.0f;

    // Setup VBOs
    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, 800,
            spacing, 800,
            spacing, 0.0f,
    };
    glGenBuffers(1, &sVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    shader = loadShaders("resources/shaders/SimpleVertexShader.glsl", "resources/shaders/SimpleFragmentShader.glsl");

    PaError err = Pa_Initialize();
    if( err != paNoError ) {
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
        return;
    }

    successInit = true;

    int numDevices;
    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
        err = numDevices;
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
        return;
    }

    const   PaDeviceInfo *deviceInfo;
    for( int i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );
        std::cout << "Name: " << deviceInfo->name << std::endl;
        std::cout << "Input channels: " << deviceInfo->maxInputChannels << std::endl;
        std::cout << "Output channels: " << deviceInfo->maxOutputChannels << std::endl;
        std::cout << "~~~" << std::endl;
    }

    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,          /* no input channels */
                                2,          /* stereo output */
                                paFloat32,  /* 32 bit floating point output */
                                SAMPLE_RATE,
                                256,        /* frames per buffer, i.e. the number
                                                   of sample frames that PortAudio will
                                                   request from the callback. Many apps
                                                   may want to use
                                                   paFramesPerBufferUnspecified, which
                                                   tells PortAudio to pick the best,
                                                   possibly changing, buffer size.*/
                                patestCallback, /* this is your callback function */
                                &data ); /*This is a pointer that will be passed to
                                                   your callback*/
    if( err != paNoError ) {
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
        return;
    }

//    err = Pa_StartStream( stream );
//    if( err != paNoError ) {
//        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
//        return;
//    }
}

void AudioAnalyzer::shutDown() {
    if (!successInit) return;

    PaError err = Pa_StopStream( stream );
    if( err != paNoError ) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return;
    }

    err = Pa_CloseStream( stream );
    if( err != paNoError ) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return;
    }

    err = Pa_Terminate();
    if( err != paNoError )
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
}

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */

    for( i=0; i<framesPerBuffer; i++ )
    {
        *out++ = data->left_phase;  /* left */
        *out++ = data->right_phase;  /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += 0.003f;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
    }
    return 0;
}

void AudioAnalyzer::render(glm::mat4 transform) {
//    float val = (avgVal - 155) / 255.0f;
//    float color[] = {val, 0.0f, 1.0f, 0.0f};
//    setColor(color);

    glUseProgram(shader);

    for (int i = 0; i < 256; i++) {
//        float val = ((int) *(audio_pos + i * 8)) / 255.0f;
//        float val = 0.5f;
        float val = data.left_phase;
        float color[] = {val, 0.0f, 0.0f, 0.0f};
        setColor(color);

        glm::mat4 translate = glm::translate(glm::vec3(i * 4, 0, 0.0f));
        drawSquare(transform * translate, true);
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
