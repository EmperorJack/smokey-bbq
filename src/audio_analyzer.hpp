#ifndef AUDIO_ANALYZER_HPP
#define AUDIO_ANALYZER_HPP

#include <opengl.hpp>
#include <portaudio.h>
#include <kiss_fftr.h>

class AudioAnalyzer {

public:

    // Constants
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int SAMPLE_SIZE = 2048;
    static constexpr int NUM_BANDS = 18;

    // Variables
    float FREQUENCY_DAMPING;

    // Setup
    AudioAnalyzer();
    void setDefaultVariables();
    void setDefaultToggles();
    void resetBuffers();
    void shutDown();

    // Audio devices
    std::vector<std::pair<int, const char*>> getInputDevices();
    bool openDevice(int deviceIndex);
    void printAudioDevices();

    // Updating
    void update();

    // Data
    float getFrequencyBand(int i);

    // Rendering
    void render(glm::mat4 transform);

    // Toggle variables
    bool displayWaveform;
    bool displaySpectrum;
    bool displayFrequencyBands;
    bool updateAnalyzer;
    bool logScaleBands;

private:

    // VBOs
    GLuint waveformVBO;
    GLuint squareVBO;

    // Shaders
    GLuint shader;

    // Instance variables
    float spacing;
    float bandSpacing;

    // Port audio variables
    bool paInitSuccessful = false;
    PaStream *stream = nullptr;
    PaStreamParameters inputParameters;

    // Audio data variables
    float processedAudio[AudioAnalyzer::SAMPLE_SIZE / 2];
    float frequencyBands[AudioAnalyzer::NUM_BANDS];
    int mapping[SAMPLE_SIZE / 2];

    // FFT variables
    kiss_fftr_cfg fft_cfg;
    kiss_fft_scalar fft_in[AudioAnalyzer::SAMPLE_SIZE];
    kiss_fft_cpx fft_out[AudioAnalyzer::SAMPLE_SIZE];

    // Setup
    void computeHanningWindow();
    void computeLogMapping();

    // Error handling
    bool paErrorOccured(PaError error);

    // Rendering
    void renderWaveform(glm::mat4);
    void renderLinearSpectrum(glm::mat4);
    void renderLogSpectrum(glm::mat4);
    void renderFrequencyBands(glm::mat4);
    void drawSquare(glm::mat4, bool);

};

#endif
