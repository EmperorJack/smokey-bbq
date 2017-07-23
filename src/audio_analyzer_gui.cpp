#include <audio_analyzer_gui.hpp>
#include <imgui.h>

std::vector<std::pair<int, const char*>> inputDevices;

AudioAnalyzerGui::AudioAnalyzerGui(AudioAnalyzer *audioAnalyzer) {
    this->audioAnalyzer = audioAnalyzer;
    inputDevices = audioAnalyzer->getInputDevices();
}

void AudioAnalyzerGui::render() {
    ImGui::Begin("Audio Analyzer");

    renderDeviceSelector();
    ImGui::Separator();
    renderToggles();
    ImGui::Separator();
    renderVariables();

    ImGui::End();
}

void AudioAnalyzerGui::renderDeviceSelector() {
    ImGui::Text("Input Devices");

    static int select = 0;

    for (int i = 0; i < inputDevices.size(); i++) {
        ImGui::RadioButton(inputDevices[i].second, &select, inputDevices[i].first);
    }

    if (ImGui::Button("Apply")) audioAnalyzer->openDevice(select);
}

void AudioAnalyzerGui::renderToggles() {
    ImGui::Checkbox("Display Waveform", &audioAnalyzer->displayWaveform);
    ImGui::Checkbox("Display Frequency Spectrum", &audioAnalyzer->displaySpectrum);
    ImGui::Checkbox("Display Frequency Bands", &audioAnalyzer->displayFrequencyBands);
    ImGui::Checkbox("Display Volume Level", &audioAnalyzer->displayVolumeLevel);
    ImGui::Checkbox("Update Analyzer", &audioAnalyzer->updateAnalyzer);
    ImGui::Checkbox("Log Scale Bands", &audioAnalyzer->logScaleBands);

    ImGui::Separator(); // Reset toggles

    if (ImGui::Button("Reset Toggles")) audioAnalyzer->setDefaultToggles();
}

void AudioAnalyzerGui::renderVariables() {
    ImGui::Text("Frequency Damping");
    ImGui::SliderFloat("##FREQUENCY_DAMPING", &audioAnalyzer->FREQUENCY_DAMPING, 0.5f, 1.0f, "%.3f");

    ImGui::Text("Frequency Band Scale");
    ImGui::SliderFloat("##FREQUENCY_SCALE", &audioAnalyzer->FREQUENCY_SCALE, 0.01f, 1.0f, "%.3f");

    ImGui::Separator(); // Reset variables

    if (ImGui::Button("Reset Variables")) audioAnalyzer->setDefaultVariables();
}
