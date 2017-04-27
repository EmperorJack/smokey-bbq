#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <vector>
#include <iostream>

const int GRID_SIZE = 15;

glm::vec2 velocityField[GRID_SIZE][GRID_SIZE];
GLuint squareVBO;
GLuint velocityVBO;

float gridSpacing;

SmokeSimulation::SmokeSimulation(float gridWorldSize) {
    gridSpacing = gridWorldSize / GRID_SIZE;
    setupVelocityField();

    // Setup VBOs
    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, gridSpacing,
            gridSpacing, gridSpacing,
            gridSpacing, 0.0f,
    };

    glGenBuffers(1, &squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STREAM_DRAW);

    float vectorVertices[] = {
            0.0f, 0.0f,
            gridSpacing / 2.0f, 0.0f,
    };

    glGenBuffers(1, &velocityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vectorVertices), vectorVertices, GL_STATIC_DRAW);
}

void SmokeSimulation::setupVelocityField() {
    // Setup 2D vectors
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            velocityField[i][j] = glm::normalize(glm::vec2(myRandom(), myRandom()));
        }
    }
}

void SmokeSimulation::update() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            velocityField[i][j] = glm::rotate(velocityField[i][j], (float) M_PI / 60.0f );
        }
    }
}

void SmokeSimulation::render(glm::mat4 transform) {
    float color[] = { 0.5f, 0.5f, 0.5f, 0.0f };
    GLint cLoc = glGetUniformLocation(3, "fillColor");
    glProgramUniform4fv(3, cLoc, GL_TRUE, color);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            drawSquare(transform * translate);

            translate *= glm::translate(glm::vec3(gridSpacing / 2.0f, gridSpacing / 2.0f, 0.0f));

            glm::vec2 velocity = velocityField[i][j];
            glm::mat4 rotation = glm::orientation(glm::vec3(velocity, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            drawVector(transform * translate * rotation);
        }
    }
}

void SmokeSimulation::drawSquare(glm::mat4 transform) {
    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
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

void SmokeSimulation::drawVector(glm::mat4 transform) {
    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_LINE_STRIP, 0, 2);
    glDisableVertexAttribArray(0);
}

float SmokeSimulation::myRandom() {
    return std::rand() % 100 / 100.0f;
}
