//
// Created by Jack Purvis
//

#include <manager.hpp>
#include <compositions/horizontal_spectrum.hpp>
#include <compositions/circular_spectrum.hpp>

Manager::Manager() {

    // Setup object instances
    smokeSimulation = new SmokeSimulation();
    audioAnalyzer = new AudioAnalyzer();

    // Setup compositions
    currentComposition = 1;
    compositions.push_back(new HorizontalSpectrum(smokeSimulation, audioAnalyzer));
    compositions.push_back(new CircularSpectrum(smokeSimulation, audioAnalyzer));

    for (Composition* composition : compositions) {
        composition->initialize();
    }
    compositions[currentComposition]->enable();

    // Setup toggles
    enableCompositions = true;
    printFrameTimes = false;
}

Manager::~Manager() {
    audioAnalyzer->shutDown();

    delete smokeSimulation;
    delete audioAnalyzer;
    for (Composition* composition : compositions) {
        delete composition;
    }
}

void Manager::update(bool mouseDragging, glm::vec2 mousePosition) {
    if (enableCompositions) {
        compositions[currentComposition]->update();
    }

    if (mouseDragging) smokeSimulation->addPulse(mousePosition);

    smokeSimulation->update();

    glm::mat4 projection = glm::ortho(0.0f, (float) SCREEN_WIDTH, (float) SCREEN_HEIGHT, 0.0f);
    glm::mat4 view = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 mvp = projection * view;

    smokeSimulation->render(mvp, mousePosition);

    audioAnalyzer->update();

    audioAnalyzer->render(mvp);
}

void Manager::setComposition(int composition) {
    currentComposition = composition;
    compositions[currentComposition]->enable();
}

void Manager::resetComponents() {
    smokeSimulation->reset();
    audioAnalyzer->resetBuffers();
}
