#ifndef AUDIO_ANALYZER_HPP
#define AUDIO_ANALYZER_HPP

#include <portaudio.h>
#include <kiss_fftr.h>

class AudioAnalyzer {

    public:

        // Constants
        static constexpr int SAMPLE_RATE = 44100;
        static constexpr int SAMPLE_SIZE = 2048;
        static constexpr int NUM_BANDS = 18;
        static constexpr float FREQUENCY_DAMPING = 0.85f;

        // Setup
        AudioAnalyzer();
        void resetBuffers();
        void shutDown();

        // Updating
        void update();

        // Data
        float getFrequencyBand(int i);

        // Rendering
        void renderWaveform(glm::mat4);
        void renderLinearSpectrum(glm::mat4);
        void renderLogSpectrum(glm::mat4);
        void renderFrequencyBands(glm::mat4);

        // Utility
        void printAudioDevices();

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
        PaStream *stream;
        PaStreamParameters outputParameters;
        PaStreamParameters inputParameters;

        // Audio data variables
        float processedAudio[AudioAnalyzer::SAMPLE_SIZE / 2];
        float frequencyBands[AudioAnalyzer::NUM_BANDS];
        int mapping[SAMPLE_SIZE / 2];

        // FFT variables
        kiss_fftr_cfg cfg;
        kiss_fft_scalar fft_in[AudioAnalyzer::SAMPLE_SIZE];
        kiss_fft_cpx fft_out[AudioAnalyzer::SAMPLE_SIZE];

        // Setup
        void computeHanningWindow();
        void computeLogMapping();

        // Error handling
        bool paErrorOccured(PaError error);

        // Rendering
        void drawSquare(glm::mat4, bool);
};

#endif
