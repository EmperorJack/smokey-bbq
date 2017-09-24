//
// Created by Jack Purvis
//

#include <compositions/horizontal_spectrum.hpp>
#include <iostream>

std::string HorizontalSpectrum::fragmentShaderPath() {
    return "compositions/SmokeFragmentShader";
}

std::vector<SmokeSimulation::Display> HorizontalSpectrum::displayFields() {
    return std::vector<SmokeSimulation::Display> {
        SmokeSimulation::Display::DENSITY,
        SmokeSimulation::Display::TEMPERATURE
    };
}

void HorizontalSpectrum::setup() {
    sideOffset = ((float) SCREEN_WIDTH * 0.08f);
    bandSpacing = ((float) SCREEN_WIDTH - sideOffset * 2.0f) / (AudioAnalyzer::NUM_BANDS * 2);
}

void HorizontalSpectrum::update() {
    float volume = max(audioAnalyzer->getOverallVolume(), 1.0f);

    for (int i = 0; i < AudioAnalyzer::NUM_BANDS * 2; i++) {
        int index;
        if (i < AudioAnalyzer::NUM_BANDS) {
            index = AudioAnalyzer::NUM_BANDS - i - 1;
        } else {
            index = i % AudioAnalyzer::NUM_BANDS;
        }

        float value = audioAnalyzer->getFrequencyBand(index);

        if (value < 3.0f) continue;

        glm::vec2 position = vec2(i * bandSpacing + sideOffset, SCREEN_HEIGHT * 0.95f);
        glm::vec2 force = vec2(myRandom() * 1.0f - 0.5f, -1.0f) * (volume + value + 0.5f) * 7.0f;
        float diameter = bandSpacing * 0.5f + value * 0.6f;
        float density = value * 0.0065f;
        float temperature = value * 0.02f;

        smokeSimulation->emit(position, diameter,
                              std::vector<SmokeSimulation::Display> { SmokeSimulation::VELOCITY, SmokeSimulation::DENSITY, SmokeSimulation::TEMPERATURE },
                              std::vector<glm::vec3> { glm::vec3(force, 0.0f), glm::vec3(density, 0.0f, 0.0f), glm::vec3(temperature, 0.0f, 0.0f) }
        );
    }
}
