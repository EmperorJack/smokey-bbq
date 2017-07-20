#include <smoke_simulation_gui.hpp>
#include <imgui.h>

SmokeSimulationGui::SmokeSimulationGui(SmokeSimulation *smokeSimulation) {
    this->smokeSimulation = smokeSimulation;
}

void SmokeSimulationGui::render() {
    ImGui::Begin("Smoke Simulation");

    renderToggles();
    ImGui::Separator();
    renderVariables();

    ImGui::End();
}

void SmokeSimulationGui::renderToggles() {
    ImGui::Checkbox("Display Density Field", &smokeSimulation->displayDensityField);
    ImGui::Checkbox("Display Velocity Field", &smokeSimulation->displayVelocityField);
    ImGui::Checkbox("Update Simulation", &smokeSimulation->updateSimulation);
    ImGui::Checkbox("Enable Emitter", &smokeSimulation->enableEmitter);
    ImGui::Checkbox("Random Impulse Angle", &smokeSimulation->randomPulseAngle);
    ImGui::Checkbox("Enable Buoyancy Force", &smokeSimulation->enableBuoyancy);
    ImGui::Checkbox("Enable Vorticity Confinement", &smokeSimulation->enableVorticityConfinement);
    ImGui::Checkbox("Wrap Borders", &smokeSimulation->wrapBorders);
    ImGui::Checkbox("Enable Pressure Solver", &smokeSimulation->enablePressureSolve);

    ImGui::Separator(); // Reset toggles

    if (ImGui::Button("Reset Toggles")) smokeSimulation->setDefaultToggles();
}

void SmokeSimulationGui::renderVariables() {

    if (ImGui::CollapsingHeader("Core variables")) {

        ImGui::Text("Time Step");
        ImGui::SliderFloat("##TIME_STEP", &smokeSimulation->TIME_STEP, 0.01f, 1.0f, "%.3f");

        // ImGui::Text("Fluid Density");
        // ImGui::SliderFloat("##FLUID_DENSITY", &smokeSimulation->FLUID_DENSITY, 0.0f, 1.0f, "%.3f");

        ImGui::Text("Jacobi Iterations");
        ImGui::SliderInt("##JACOBI_ITERATIONS", &smokeSimulation->JACOBI_ITERATIONS, 0, 100, "%.0f");
    }

    if (ImGui::CollapsingHeader("Force Variables")) {

        // ImGui::Text("Gravity");
        // ImGui::SliderFloat("##GRAVITY", &smokeSimulation->GRAVITY, -5.0f, 5.0f, "%.3f");

        ImGui::Text("Pulse Range");
        ImGui::SliderFloat("##PULSE_RANGE", &smokeSimulation->PULSE_RANGE, 0.0f, 500.0f, "%.1f");

        ImGui::Text("Emitter Range");
        ImGui::SliderFloat("##EMITTER_RANGE", &smokeSimulation->EMITTER_RANGE, 0.0f, 500.0f, "%.1f");

        ImGui::Text("Pulse Force");
        ImGui::SliderFloat("##PULSE_FORCE", &smokeSimulation->PULSE_FORCE, 0.0f, 500.0f, "%.1f");

        ImGui::Text("Vorticity Confinement Force");
        ImGui::SliderFloat("##VORTICITY_CONFINEMENT_FORCE", &smokeSimulation->VORTICITY_CONFINEMENT_FORCE, 0.0f, 10.0f, "%.2f");
    }

    if (ImGui::CollapsingHeader("Dissipation variables")) {

        ImGui::Text("Velocity Dissipation");
        ImGui::SliderFloat("##VELOCITY_DISSIPATION", &smokeSimulation->VELOCITY_DISSIPATION, 0.8f, 1.0f, "%.3f");

        ImGui::Text("Density Dissipation");
        ImGui::SliderFloat("##DENSITY_DISSIPATION", &smokeSimulation->DENSITY_DISSIPATION, 0.8f, 1.0f, "%.3f");

        ImGui::Text("Temperature Dissipation");
        ImGui::SliderFloat("##TEMPERATURE_DISSIPATION", &smokeSimulation->TEMPERATURE_DISSIPATION, 0.8f, 1.0f, "%.3f");

    }

    if (ImGui::CollapsingHeader("Buoyancy variables")) {

        ImGui::Text("Rise Force");
        ImGui::SliderFloat("##RISE_FORCE", &smokeSimulation->RISE_FORCE, 0.1f, 10.0f, "%.2f");

        ImGui::Text("Fall Force");
        ImGui::SliderFloat("##FALL_FORCE", &smokeSimulation->FALL_FORCE, 0.1f, 10.0f, "%.2f");

        ImGui::Text("Atmosphere Temperature");
        ImGui::SliderFloat("##ATMOSPHERE_TEMPERATURE", &smokeSimulation->ATMOSPHERE_TEMPERATURE, -10.0f, 10.0f, "%.2f");

    }

    // ImGui::Text("Stroke Weight");
    // ImGui::SliderFloat("##O", &smokeSimulation->STROKE_WEIGHT, 0.1f, 10.0f, "%.2f");

    ImGui::Separator(); // Reset variables

    if (ImGui::Button("Reset Variables")) smokeSimulation->setDefaultVariables();
}
