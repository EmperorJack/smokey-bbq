//
// Created by Jack Purvis
//

#include <cmath>
#include <compositions/circular_spectrum.hpp>

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
    radius = SCREEN_WIDTH * 0.05f;
    screenCenter = glm::vec2(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f);
    globalRotation = 0.0f;
}

void CircularSpectrum::update() {
    renderHalfCircle(globalRotation);
    renderHalfCircle(globalRotation + (float) M_PI);

    globalRotation += (float) M_PI / 180.0f * audioAnalyzer->getOverallVolume() / 4.0f;
}

void CircularSpectrum::renderHalfCircle(float flip) {
    float volume = audioAnalyzer->getOverallVolume();

    for (int i = 0; i < AudioAnalyzer::NUM_BANDS; i++) {
        float angle = angleSpacing * i + flip;
        float value = audioAnalyzer->getFrequencyBand(i);

        if (value < 3.0f) continue;

        float x = sin(angle) * (radius / 2.0f * volume / 10.0f + radius);// * flip;
        float y = -cos(angle) * (radius / 2.0f * volume / 10.0f + radius);

        glm::vec2 position = vec2(x, y) + screenCenter;
        glm::vec2 force = vec2(myRandom() * 1.0f - 0.5f, -1.0f) * (volume + value + 0.5f) * 7.0f;
        force = glm::rotate(force, angle);// * vec2(flip, 1.0f);

        float diameter = radius * 0.05f + value * 0.6f;
        float density = value * 0.0065f;
        float temperature = value * 0.02f;

        smokeSimulation->emit(position, force, diameter, density, temperature);
    }
}
