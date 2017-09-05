//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_HORIZONTAL_SPECTRUM_HPP
#define SMOKEYBBQ_HORIZONTAL_SPECTRUM_HPP

#include <compositions/composition.hpp>

class HorizontalSpectrum : public Composition {

public:
    HorizontalSpectrum(SmokeSimulation* smokeSimulation, AudioAnalyzer* audioAnalyzer, std::string fragmentShaderPath) :
        Composition(smokeSimulation, audioAnalyzer, fragmentShaderPath) {}
    void update();

};

#endif //SMOKEYBBQ_HORIZONTAL_SPECTRUM_HPP
