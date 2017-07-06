#ifndef SMOKEYBBQ_AUDIO_ANALYZER_GUI_HPP
#define SMOKEYBBQ_AUDIO_ANALYZER_GUI_HPP

#include <audio_analyzer.hpp>

class AudioAnalyzerGui {

public:

    // Setup
    AudioAnalyzerGui(AudioAnalyzer *audioAnalyzer);

    // Rendering
    void render();

private:

    // Smoke simulation instance
    AudioAnalyzer *audioAnalyzer;

};


#endif
