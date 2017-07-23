#ifndef SMOKE_SIMULATION_GUI_HPP
#define SMOKE_SIMULATION_GUI_HPP

#include <smoke_simulation.hpp>

class SmokeSimulationGui {

public:

    // Setup
    SmokeSimulationGui(SmokeSimulation *smokeSimulation);

    // Rendering
    void render();

private:

    // Smoke simulation instance
    SmokeSimulation *smokeSimulation;

    // Rendering
    void renderToggles();
    void renderDisplaySelector();
    void renderVariables();

};


#endif
