//
// Created by Jack Purvis
//

#ifndef SMOKEYBBQ_MANAGER_HPP
#define SMOKEYBBQ_MANAGER_HPP

#include <smoke_simulation/smoke_simulation.hpp>
#include <audio_analyzer/audio_analyzer.hpp>
#include <compositions/composition.hpp>

class Manager {

public:

    // Composition toggle
    int currentComposition;

    // Toggle variables
    bool enableCompositions;
    bool printFrameTimes;

    // Components
    SmokeSimulation* smokeSimulation;
    AudioAnalyzer* audioAnalyzer;

    // Compositions
    std::vector<Composition*> compositions;

    // Setup
    Manager();
    ~Manager();

    // Core
    void update(bool mouseDragging, glm::vec2 mousePosition);
    void setComposition(int composition);
    void resetComponents();

};

#endif //SMOKEYBBQ_MANAGER_HPP
