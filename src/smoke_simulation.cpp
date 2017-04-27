#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <iostream>

const int GRID_SIZE = 15;

glm::vec2 u[GRID_SIZE][GRID_SIZE];

float gridSpacing;

SmokeSimulation::SmokeSimulation(float size) {
    gridSpacing = size / GRID_SIZE;
}

void SmokeSimulation::update() {

}

void SmokeSimulation::render() {
    float color[] = { 0.5f, 0.5f, 0.5f, 0.0f };
    GLint cLoc = glGetUniformLocation(3, "fillColor");
    glProgramUniform4fv(3, cLoc, GL_TRUE, color);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;

            drawSquare(x, y, gridSpacing);
        }
    }
}

void SmokeSimulation::drawSquare(float x, float y, float size) {
    float vertices[] = {
            x, y,
            x, y + size,
            x + size, y + size,
            x + size, y,
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableVertexAttribArray(0);
}
