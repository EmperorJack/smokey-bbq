#include <imgui.h>
#include <smoke_simulation/smoke_simulation_gui.hpp>

SmokeSimulationGui::SmokeSimulationGui(SmokeSimulation *smokeSimulation) :
    smokeSimulation(smokeSimulation) {
    displaySelect = smokeSimulation->currentDisplay;
}

void SmokeSimulationGui::render() {
    ImGui::Begin("Smoke Simulation");

    renderToggles();
    ImGui::Separator();
    renderDisplaySelector();
    ImGui::Separator();
    renderVariables();

    ImGui::End();
}

void SmokeSimulationGui::renderToggles() {
    ImGui::Checkbox("Display Density Field", &smokeSimulation->displaySmokeField);
    ImGui::Checkbox("Display Velocity Field", &smokeSimulation->displayVelocityField);
    ImGui::Checkbox("Update Simulation", &smokeSimulation->updateSimulation);
    ImGui::Checkbox("Enable Emitter", &smokeSimulation->enableEmitter);
    ImGui::Checkbox("Random Impulse Angle", &smokeSimulation->randomPulseAngle);
    ImGui::Checkbox("Enable Buoyancy Force", &smokeSimulation->enableBuoyancy);
    ImGui::Checkbox("Enable Vorticity Confinement", &smokeSimulation->enableVorticityConfinement);
    ImGui::Checkbox("Wrap Borders", &smokeSimulation->wrapBorders);
    ImGui::Checkbox("Enable Pressure Solver", &smokeSimulation->enablePressureSolver);
    ImGui::Checkbox("Compute Intermediate Fields", &smokeSimulation->computeIntermediateFields);
    ImGui::Checkbox("CPU Multithreading", &smokeSimulation->useCPUMultithreading);
    ImGui::Checkbox("GPU Implementation", &smokeSimulation->useGPUImplementation);

    ImGui::Separator(); // Reset toggles

    if (ImGui::Button("Reset Toggles")) smokeSimulation->setDefaultToggles();
}

void SmokeSimulationGui::renderDisplaySelector() {
    ImGui::Text("Display Fields");

    ImGui::RadioButton("Composition", &displaySelect, SmokeSimulation::COMPOSITION);
    ImGui::RadioButton("Density", &displaySelect, SmokeSimulation::DENSITY);
    ImGui::RadioButton("Velocity", &displaySelect, SmokeSimulation::VELOCITY);
    ImGui::RadioButton("Temperature", &displaySelect, SmokeSimulation::TEMPERATURE);
    ImGui::RadioButton("Curl", &displaySelect, SmokeSimulation::CURL);

    if (ImGui::Button("Apply")) smokeSimulation->currentDisplay = SmokeSimulation::Display(displaySelect);
}

void SmokeSimulationGui::renderVariables() {

    if (ImGui::CollapsingHeader("Core variables")) {

        ImGui::Text("Time Step");
        ImGui::SliderFloat("##timeStep", &smokeSimulation->timeStep, 0.01f, 0.25f, "%.3f");

        // ImGui::Text("Fluid Density");
        // ImGui::SliderFloat("##fluidDensity", &smokeSimulation->fluidDensity, 0.0f, 1.0f, "%.3f");

        ImGui::Text("Jacobi Iterations");
        ImGui::SliderInt("##jacobiIterations", &smokeSimulation->jacobiIterations, 0, 100, "%.0f");
    }

    if (ImGui::CollapsingHeader("Force Variables")) {

        // ImGui::Text("Gravity");
        // ImGui::SliderFloat("##gravity", &smokeSimulation->gravity, -5.0f, 5.0f, "%.3f");

        ImGui::Text("Pulse Range");
        ImGui::SliderFloat("##pulseRange", &smokeSimulation->pulseRange, 0.0f, 500.0f, "%.1f");

        ImGui::Text("Emitter Range");
        ImGui::SliderFloat("##emitterRange", &smokeSimulation->emitterRange, 0.0f, 500.0f, "%.1f");

        ImGui::Text("Pulse Force");
        ImGui::SliderFloat("##pulseForce", &smokeSimulation->pulseForce, 0.0f, 500.0f, "%.1f");

        ImGui::Text("Vorticity Confinement Force");
        ImGui::SliderFloat("##vorticityConfinementForce", &smokeSimulation->vorticityConfinementForce, 0.0f, 10.0f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Dissipation variables")) {

        ImGui::Text("Velocity Dissipation");
        ImGui::SliderFloat("##velocityDissipation", &smokeSimulation->velocityDissipation, 0.8f, 1.0f, "%.3f");

        ImGui::Text("Density Dissipation");
        ImGui::SliderFloat("##densityDissipation", &smokeSimulation->densityDissipation, 0.8f, 1.0f, "%.3f");

        ImGui::Text("Temperature Dissipation");
        ImGui::SliderFloat("##temperatureDissipation", &smokeSimulation->temperatureDissipation, 0.8f, 1.0f, "%.3f");

    }

    if (ImGui::CollapsingHeader("Buoyancy variables")) {

        ImGui::Text("Rise Force");
        ImGui::SliderFloat("##riseForce", &smokeSimulation->riseForce, 0.1f, 10.0f, "%.2f");

        ImGui::Text("Fall Force");
        ImGui::SliderFloat("##fallForce", &smokeSimulation->fallForce, 0.1f, 10.0f, "%.2f");

        ImGui::Text("Atmosphere Temperature");
        ImGui::SliderFloat("##atmosphereTemperature", &smokeSimulation->atmosphereTemperature, -10.0f, 10.0f, "%.2f");

    }

    // ImGui::Text("Stroke Weight");
    // ImGui::SliderFloat("##O", &smokeSimulation->strokeWeight, 0.1f, 10.0f, "%.2f");

    ImGui::Separator(); // Reset variables

    if (ImGui::Button("Reset Variables")) smokeSimulation->setDefaultVariables();

    ImGui::Separator(); // Benchmark

    if (ImGui::Button("Benchmark")) smokeSimulation->beginBenchmark();
}
