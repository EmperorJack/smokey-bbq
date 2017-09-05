//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_COMPOSITION_HPP
#define SMOKEYBBQ_COMPOSITION_HPP

#include <smoke_simulation/smoke_simulation.hpp>
#include <audio_analyzer/audio_analyzer.hpp>

class Composition {

public:
    Composition(SmokeSimulation* smokeSimulation, AudioAnalyzer* audioAnalyzer) :
            smokeSimulation(smokeSimulation), audioAnalyzer(audioAnalyzer) {}
    void initialize();
    void enable();

    virtual std::string fragmentShaderPath() {}
    virtual std::vector<SmokeSimulation::Display> displayFields() {}
    virtual void update() {}

    SmokeSimulation* smokeSimulation;
    AudioAnalyzer* audioAnalyzer;
    GLuint shader;
    std::vector<SmokeSimulation::Display> fields;

};

#endif //SMOKEYBBQ_COMPOSITION_HPP
