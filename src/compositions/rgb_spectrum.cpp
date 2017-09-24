//
// Created by Jack Purvis
//

#include <imgui.h>
#include <compositions/rgb_spectrum.hpp>

std::string RgbSpectrum::fragmentShaderPath() {
    return "compositions/RgbFragmentShader";
}

std::vector<SmokeSimulation::Display> RgbSpectrum::displayFields() {
    return std::vector<SmokeSimulation::Display> {
        SmokeSimulation::Display::RGB,
        SmokeSimulation::Display::TEMPERATURE
    };
}

void RgbSpectrum::setup() {
    sideOffset = ((float) SCREEN_WIDTH * 0.08f);
    bandSpacing = ((float) SCREEN_WIDTH - sideOffset * 2.0f) / (AudioAnalyzer::NUM_BANDS * 2);
}

glm::vec3 hue(float h) {
    float r = abs(h * 6 - 3) - 1;
    float g = 2 - abs(h * 6 - 2);
    float b = 2 - abs(h * 6 - 4);
    return glm::saturate(glm::vec3(r, g, b));
}

float map(float value, float srcMin, float srcMax, float destMin, float destMax) {
    return (value - srcMin) * (destMax - destMin) / (srcMax - srcMin) + destMin;
}

void RgbSpectrum::update() {
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
        float temperature = value * 0.02f;

        float h = map(i, 0, AudioAnalyzer::NUM_BANDS * 2, 0.0f, 1.0f);
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(h, 1.0f, 1.0f, r, g, b);

        smokeSimulation->emit(position, diameter,
                              std::vector<SmokeSimulation::Display> { SmokeSimulation::VELOCITY, SmokeSimulation::RGB, SmokeSimulation::TEMPERATURE },
                              std::vector<glm::vec3> { glm::vec3(force, 0.0f), glm::vec3(r, g, b), glm::vec3(temperature, 0.0f, 0.0f) }
        );
    }
}
