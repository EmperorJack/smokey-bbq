//
// Created by Jack Purvis
//

#include <cmath>
#include <compositions/circular_spectrum.hpp>
#include <glm/gtx/rotate_vector.hpp>

std::string CircularSpectrum::fragmentShaderPath() {
    return "compositions/SmokeFragmentShader";
}

std::vector<SmokeSimulation::Display> CircularSpectrum::displayFields() {
    return std::vector<SmokeSimulation::Display> {
        SmokeSimulation::Display::DENSITY,
        SmokeSimulation::Display::TEMPERATURE
    };
}

void CircularSpectrum::setup() {
    angleSpacing = (float) M_PI / (AudioAnalyzer::NUM_BANDS - 1);
    radius = SCREEN_WIDTH * 0.075f;
    screenCenter = glm::vec2(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f);
}

void CircularSpectrum::update() {
    renderHalfCircle(1.0f);
    renderHalfCircle(-1.0f);
}

void CircularSpectrum::renderHalfCircle(float flip) {
    float volume = audioAnalyzer->getOverallVolume();

    for (int i = 0; i < AudioAnalyzer::NUM_BANDS; i++) {
        float angle = angleSpacing * i;
        float value = audioAnalyzer->getFrequencyBand(i);

        float x = sin(angle) * radius * flip;
        float y = -cos(angle) * radius;

        glm::vec2 position = vec2(x, y) + screenCenter;
        glm::vec2 force = vec2(myRandom() * 1.0f - 0.5f, -1.0f) * (volume + value + 0.5f) * 7.0f;
        force = glm::rotate(force, angle) * vec2(flip, 1.0f);

        float diameter = radius * 0.1f + value * 0.6f;
        float density = value * 0.0065f;
        float temperature = value * 0.02f;

        smokeSimulation->emit(position, force, diameter, density, temperature);
    }
}
