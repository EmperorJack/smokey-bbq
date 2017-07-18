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
    // Core equation variables

    ImGui::Text("Time Step");
    ImGui::SliderFloat("##A", &smokeSimulation->TIME_STEP, 0.01f, 1.0f, "%.3f");

    // ImGui::Text("Fluid Density");
    // ImGui::SliderFloat("##B", &smokeSimulation->FLUID_DENSITY, 0.0f, 1.0f, "%.3f");

    ImGui::Text("Jacobi Iterations");
    ImGui::SliderInt("##C", &smokeSimulation->JACOBI_ITERATIONS, 0, 100, "%.0f");

    ImGui::Separator(); // Force variables

    // ImGui::Text("Gravity");
    // ImGui::SliderFloat("##D", &smokeSimulation->GRAVITY, -5.0f, 5.0f, "%.3f");

    ImGui::Text("Pulse Range");
    ImGui::SliderFloat("##E", &smokeSimulation->PULSE_RANGE, 0.0f, 500.0f, "%.1f");

    ImGui::Text("Emitter Range");
    ImGui::SliderFloat("##F", &smokeSimulation->EMITTER_RANGE, 0.0f, 500.0f, "%.1f");

    ImGui::Text("Pulse Force");
    ImGui::SliderFloat("##G", &smokeSimulation->PULSE_FORCE, 0.0f, 500.0f, "%.1f");

    ImGui::Separator(); // Dissipation variables

    ImGui::Text("Velocity Dissipation");
    ImGui::SliderFloat("##H", &smokeSimulation->VELOCITY_DISSIPATION, 0.8f, 1.0f, "%.3f");

    ImGui::Text("Density Dissipation");
    ImGui::SliderFloat("##I", &smokeSimulation->DENSITY_DISSIPATION, 0.8f, 1.0f, "%.3f");

    ImGui::Text("Temperature Dissipation");
    ImGui::SliderFloat("##J", &smokeSimulation->TEMPERATURE_DISSIPATION, 0.8f, 1.0f, "%.3f");

    ImGui::Separator(); // Buoyancy variables

    ImGui::Text("Rise Force");
    ImGui::SliderFloat("##K", &smokeSimulation->RISE_FORCE, 0.1f, 10.0f, "%.2f");

    ImGui::Text("Fall Force");
    ImGui::SliderFloat("##L", &smokeSimulation->FALL_FORCE, 0.1f, 10.0f, "%.2f");

    ImGui::Text("Atmosphere Temperature");
    ImGui::SliderFloat("##M", &smokeSimulation->ATMOSPHERE_TEMPERATURE, -10.0f, 10.0f, "%.2f");

    ImGui::Separator(); // Vortex confinement

    ImGui::Text("Vorticity Confinement Force");
    ImGui::SliderFloat("##N", &smokeSimulation->VORTICITY_CONFINEMENT_FORCE, 0.0f, 10.0f, "%.2f");

    // ImGui::Separator(); // Misc variables

    // ImGui::Text("Stroke Weight");
    // ImGui::SliderFloat("##O", &smokeSimulation->STROKE_WEIGHT, 0.1f, 10.0f, "%.2f");

    ImGui::Separator(); // Reset variables

    if (ImGui::Button("Reset Variables")) smokeSimulation->setDefaultVariables();
}
