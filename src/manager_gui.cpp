//
// Created by Jack Purvis
//

#include <imgui.h>
#include <manager_gui.hpp>

ManagerGui::ManagerGui(Manager* manager) :
    manager(manager) {
    this->compositionSelect = manager->currentComposition;
}

void ManagerGui::render() {
    ImGui::Begin("Main");

    renderToggles();
    ImGui::Separator();
    renderCompositionSelector();
    ImGui::Separator();

    ImGui::End();
}

void ManagerGui::renderToggles() {
    ImGui::Checkbox("Enable Compositions", &manager->enableCompositions);
    ImGui::Checkbox("Print Frame Times", &manager->printFrameTimes);

    ImGui::Separator(); // Reset toggles

    if (ImGui::Button("Reset Components")) {
        manager->resetComponents();
    }
}

void ManagerGui::renderCompositionSelector() {
    ImGui::Text("Compositions");

    ImGui::RadioButton("Horizontal Spectrum", &compositionSelect, 0);

    if (ImGui::Button("Apply")) manager->currentComposition = compositionSelect;
}
