//
// Created by Jack Purvis
//

#include <compositions/composition.hpp>
#include <shaderLoader.hpp>

void Composition::initialize() {
    shader = loadShaders("SmokeVertexShader", fragmentShaderPath());
    fields = displayFields();
}

void Composition::enable() {
    smokeSimulation->setCompositionData(shader, fields);
    setup();
}
