#include <iostream>
#include <opengl.hpp>
#include <portaudio.h>
#include <audio_analyzer.hpp>
#include <shaderLoader.hpp>
#include <string.h>

#define MUS_PATH "resources/shaders/kick.wav"

GLuint sVBO;
GLuint shader;

bool successInit = false;

#define SAMPLE_RATE (44100)
#define fpb (1024)

PaStream *stream;
PaStreamParameters outputParameters;
PaStreamParameters inputParameters;

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
    float spacing = 800 / ((float) fpb);

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
        std::cout << "Device (" << i << "):" << std::endl;
        std::cout << "Name: " << deviceInfo->name << std::endl;
        std::cout << "Input channels: " << deviceInfo->maxInputChannels << std::endl;
        std::cout << "Output channels: " << deviceInfo->maxOutputChannels << std::endl;
        std::cout << "~~~" << std::endl;
    }

    int inDevNum = 1;
    int inChan = 2;

    int outDevNum = 3;
    int outChan = 2;

    memset( &inputParameters, 0, sizeof( inputParameters ) ); //not necessary if you are filling in all the fields
    inputParameters.channelCount = inChan;
    inputParameters.device = inDevNum;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inDevNum)->defaultLowInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field

    memset( &outputParameters, 0, sizeof( outputParameters ) ); //not necessary if you are filling in all the fields
    outputParameters.channelCount = outChan;
    outputParameters.device = outDevNum;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outDevNum)->defaultLowOutputLatency ;
    outputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field
    err = Pa_OpenStream(
            &stream,
            &inputParameters,
            &outputParameters,
            SAMPLE_RATE,
            fpb,
            paNoFlag, //flags that can be used to define dither, clip settings and more
            patestCallback, //your callback function
            (void *)this
    ); //data to be passed to callback. In C++, it is frequently (void *)this
    //don't forget to check errors!

    if( err != paNoError ) {
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
        return;
    }

    err = Pa_StartStream( stream );
    if( err != paNoError ) {
        printf(  "PortAudio error: %s\n", Pa_GetErrorText( err ) );
        return;
    }
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

float vals[fpb];

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    float *in = (float*) inputBuffer;
    float *out = (float*)outputBuffer;
    unsigned int i;

    for( i=0; i<framesPerBuffer; i++ )
    {
        vals[i] = *in++;
    }

    return 0;
}

void AudioAnalyzer::render(glm::mat4 transform) {
    glUseProgram(shader);

    for (int i = 0; i < fpb; i++) {
        float red = vals[i] * 15.0f;
        float green = 0.0f;
        float blue = 0.0f;
        if (red > 0.0f) blue = red;
        else if (red < 0.0f) green = abs(red);

        float color[] = {red, green, blue, 0.0f};
        setColor(color);

        glm::mat4 translate = glm::translate(glm::vec3(i * 1, 0, 0.0f));
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
