//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_RGB_SPECTRUM_HPP
#define SMOKEYBBQ_RGB_SPECTRUM_HPP

#include <compositions/composition.hpp>

class RgbSpectrum : public Composition {

public:
    RgbSpectrum(SmokeSimulation* smokeSimulation, AudioAnalyzer* audioAnalyzer) :
        Composition(smokeSimulation, audioAnalyzer) {}
    std::string fragmentShaderPath();
    std::vector<SmokeSimulation::Display> displayFields();

    void setup();
    void update();

private:
    float sideOffset;
    float bandSpacing;

};

#endif //SMOKEYBBQ_RGB_SPECTRUM_HPP
