//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_COMPOSITION_HPP
#define SMOKEYBBQ_COMPOSITION_HPP

#include <smoke_simulation/smoke_simulation.hpp>
#include <audio_analyzer/audio_analyzer.hpp>

class Composition {

public:
    Composition(SmokeSimulation* smokeSimulation, AudioAnalyzer* audioAnalyzer, std::string fragmentShaderPath);
    void enable();
    virtual void update();

    SmokeSimulation* smokeSimulation;
    AudioAnalyzer* audioAnalyzer;
    GLuint shader;

};

#endif //SMOKEYBBQ_COMPOSITION_HPP
