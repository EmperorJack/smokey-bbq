#include <audio_analyzer_gui.hpp>
#include <imgui.h>

AudioAnalyzerGui::AudioAnalyzerGui(AudioAnalyzer *audioAnalyzer) {
    this->audioAnalyzer = audioAnalyzer;
}

void AudioAnalyzerGui::render() {
    ImGui::Begin("Audio Analyzer");

    // Toggles

    ImGui::Checkbox("Display Waveform", &audioAnalyzer->displayWaveform);
    ImGui::Checkbox("Display Frequency Spectrum", &audioAnalyzer->displaySpectrum);
    ImGui::Checkbox("Display Frequency Bands", &audioAnalyzer->displayFrequencyBands);
    ImGui::Checkbox("Update Analyzer", &audioAnalyzer->updateAnalyzer);
    ImGui::Checkbox("Log Scale Bands", &audioAnalyzer->logScaleBands);

    ImGui::Separator(); // Reset toggles

    if (ImGui::Button("Reset Toggles")) audioAnalyzer->setDefaultToggles();

    ImGui::Separator(); // Variables

    ImGui::Text("Frequency Damping");
    ImGui::SliderFloat("##A", &audioAnalyzer->FREQUENCY_DAMPING, 0.5f, 1.0f, "%.3f");

    ImGui::Separator(); // Reset variables

    if (ImGui::Button("Reset Variables")) audioAnalyzer->setDefaultVariables();

    ImGui::End();
}
