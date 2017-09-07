//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_CIRCULAR_SPECTRUM_HPP
#define SMOKEYBBQ_CIRCULAR_SPECTRUM_HPP

#include <compositions/composition.hpp>

class CircularSpectrum : public Composition {

public:
    CircularSpectrum(SmokeSimulation* smokeSimulation, AudioAnalyzer* audioAnalyzer) :
        Composition(smokeSimulation, audioAnalyzer) {}
    std::string fragmentShaderPath();
    std::vector<SmokeSimulation::Display> displayFields();
    void setup();
    void update();

private:
    float angleSpacing;
    float radius;
    glm::vec2 screenCenter;
    float globalRotation;

    void renderHalfCircle(float flip);

};

#endif //SMOKEYBBQ_CIRCULAR_SPECTRUM_HPP
