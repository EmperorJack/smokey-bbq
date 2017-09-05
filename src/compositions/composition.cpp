//
// Created by Jack Purvis
//

#include <string>
#include <compositions/composition.hpp>
#include <shaderLoader.hpp>
#include <iostream>
Composition::Composition(SmokeSimulation* smokeSimulation, AudioAnalyzer* audioAnalyzer, std::string fragmentShaderPath) :
    smokeSimulation(smokeSimulation), audioAnalyzer(audioAnalyzer) {
    shader = loadShaders("SmokeVertexShader", fragmentShaderPath);
    enable();
}

void Composition::enable() {
    smokeSimulation->setCompositionShader(shader);
}

void Composition::update() {
    // Implement me
}
