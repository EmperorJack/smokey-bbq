#include <iostream>
#include <SDL2/SDL.h>
#include <opengl.hpp>
#include <audio_analyzer.hpp>
#include <shaderLoader.hpp>

#define MUS_PATH "resources/shaders/kick.wav"

// prototype for our audio callback
// see the implementation for more information
void my_audio_callback(void *userdata, Uint8 *stream, int len);

// variable declarations
static Uint8 *audio_pos; // global pointer to the audio buffer to be played
static Uint32 audio_len; // remaining length of the sample we have to play

static Uint8 *wav_buffer; // buffer containing our audio file

SDL_AudioDeviceID dev;
SDL_AudioSpec want, have;

int avgVal = 0;

bool paused = true;
bool ended = false;

GLuint sVBO;
GLuint shader;

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

    // Initialize SDL.
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cout << "Failed to initialize audio" << std::endl;
        return;
    }

    int devcount = SDL_GetNumAudioDevices(SDL_TRUE);
    for (int i = 0; i < devcount; i++) {
        SDL_Log(" Capture device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
    }
    std::cout << "~~~~~" << std::endl << devcount << std::endl;

    // local variables
//    static Uint32 wav_length; // length of our sample
//    static SDL_AudioSpec wav_spec; // the specs of our piece of music
//
//
//    /* Load the WAV */
//    // the specs, length and buffer of our wav are filled
//    if( SDL_LoadWAV(MUS_PATH, &wav_spec, &wav_buffer, &wav_length) == NULL ){
//        std::cout << "Failed to load wav file" << std::endl;
//        return;
//    }
//    // set the callback function
//    wav_spec.callback = my_audio_callback;
//    wav_spec.userdata = NULL;
//    wav_spec.samples = 512;
////    wav_spec.format = AUDIO_F32;
//    // set our global static variables
//    audio_pos = wav_buffer; // copy sound buffer
//    audio_len = wav_length; // copy file length
//
//    /* Open the audio device */
//    if ( SDL_OpenAudio(&wav_spec, NULL) < 0 ){
//        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
//        exit(-1);
//    }
//
//    /* Start playing */
//    togglePlay();

    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = my_audio_callback;
    dev = SDL_OpenAudioDevice(NULL, SDL_TRUE, &want, &have, 0); //SDL_GetAudioDeviceName(0, 1)

    if (have.format != want.format) {
        SDL_Log("We didn't get the wanted format.");
        std::cout << "Wanted: " << want.format << std::endl;
        std::cout << "Got: " << have.format << std::endl;
        //return;
    }

    std::cout << "Got a device with state: " << dev << std::endl;

//    SDL_PauseAudioDevice(dev, 0);

    if (dev == 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return;
    }

    printf("Started at %u\n", SDL_GetTicks());
    //SDL_Delay(5000);

    //SDL_CloseAudioDevice(dev);
}

void AudioAnalyzer::togglePlay() {
    if (paused) {
        SDL_PauseAudio(0);
        paused = false;
    } else {
        SDL_PauseAudio(1);
        paused = true;
    }
}

void AudioAnalyzer::checkEnded() {
    // wait until we're don't playing
    if ( audio_len <= 0 ) {

        if (!ended) {
            // shut everything down
            SDL_CloseAudio();
            SDL_FreeWAV(wav_buffer);
            ended = true;
        }
    }
}

// audio callback function
// here you have to copy the data of your audio buffer into the
// requesting audio buffer (stream)
// you should only copy as much as the requested length (len)
void my_audio_callback(void *userdata, Uint8 *stream, int len) {
//    printf("Callback at %u\n", SDL_GetTicks());

    std::cout << ((int) *stream) << std::endl;
//
//    if (audio_len == 0)
//        return;
//
//    len = ( len > audio_len ? audio_len : len );
//    SDL_memcpy (stream, audio_pos, len); 					// simply copy from one buffer into the other
////    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);// mix from one buffer into another
//
////    int lastAvgVal = avgVal;
////
////    avgVal = 0;
////    for (int i = 0; i < len; i++) {
////        avgVal += (int) *audio_pos;
////    }
////    avgVal /= len;
////
////    avgVal = (avgVal + lastAvgVal) / 2;
//
//    audio_pos += len;
//    audio_len -= len;
}

void AudioAnalyzer::render(glm::mat4 transform) {
//    float val = (avgVal - 155) / 255.0f;
//    float color[] = {val, 0.0f, 1.0f, 0.0f};
//    setColor(color);

    if (ended) return;

    glUseProgram(shader);

    for (int i = 0; i < 128; i++) {
        float val = ((int) *(audio_pos + i * 8)) / 255.0f;
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