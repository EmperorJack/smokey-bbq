#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H

class AudioAnalyzer {

    public:

    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int SAMPLE_SIZE = 1024;
    static constexpr int FFT_SIZE = 1024;

    AudioAnalyzer(float screenWidth, float screenHeight);
    void shutDown();

    void computeHanningWindow();
    void printAudioDevices();

    void update();

    void renderWaveform(glm::mat4);
    void renderSpectrum(glm::mat4);
    void renderFrequencyBands(glm::mat4);
    void drawSquare(glm::mat4, bool);
};

#endif
