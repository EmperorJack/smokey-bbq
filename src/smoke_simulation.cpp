#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <iostream>

const int GRID_SIZE = 25;
const float TIME_STEP = 10.0f;
const bool WRAP_BORDERS = false;

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
            0.0f, gridSpacing / 2.5f,
    };
    glGenBuffers(1, &velocityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vectorVertices), vectorVertices, GL_STATIC_DRAW);
}

void SmokeSimulation::setupVelocityField() {
    // Setup 2D vectors
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing - GRID_SIZE * gridSpacing / 2.0f; // Adding a small offset here to prevent 0 length vectors
            float y = j * gridSpacing - GRID_SIZE * gridSpacing / 2.0f; // 0.0000000000001f
            velocityField[i][j] = glm::normalize(glm::vec2(x, y));
            // sin(2 * M_PI * y), sin(2 * M_PI * x)
            // y, x
            // myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f
        }
    }
}

void SmokeSimulation::update() {
    glm::vec2 uAdvected[GRID_SIZE][GRID_SIZE];

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            uAdvected[i][j] = advectedVelocityAt(i, j);
            // velocityField[i][j] = glm::rotate(velocityField[i][j], (float) M_PI / 120.0f );
        }
    }

    std::copy(&uAdvected[0][0], &uAdvected[0][0]+GRID_SIZE*GRID_SIZE, &velocityField[0][0]);
}

glm::vec2 SmokeSimulation::advectedVelocityAt(int i, int j) {
    glm::vec2 cellPosition = glm::vec2(i * gridSpacing, j * gridSpacing);
    glm::vec2 tracePosition = traceParticle(cellPosition.x, cellPosition.y, TIME_STEP);

    return getVelocity(tracePosition.x, tracePosition.y);
}

glm::vec2 SmokeSimulation::traceParticle(float x, float y, float timeStep) {
    return glm::vec2(x, y) + (timeStep * getVelocity(x, y));
//    glm::vec2 cellPosition = glm::vec2(x, y);
//    glm::vec2 rkPos = cellPosition + timeStep / 2 * getVelocity(x, y);
//    return cellPosition + (timeStep * getVelocity(rkPos.x, rkPos.y));

    // Euler method:         y = x + ∆t * u(x)
    // Ranga kutta 2 method: y = x + ∆t * u(x + ∆t / 2 * u(x))
}

glm::vec2 SmokeSimulation::getVelocity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    glm::vec2 v = glm::vec2();
    v.x = getInterpolatedValue(normX, normY, 0);
    v.y = getInterpolatedValue(normX, normY, 1);

    return v;
}

float SmokeSimulation::getInterpolatedValue(float x, float y, int index) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getCellVelocity(i, j)[index] +
           (x-i) * (j+1-y)   * getCellVelocity(i+1, j)[index] +
           (i+1-x) * (y-j)   * getCellVelocity(i, j+1)[index] +
           (x-i) * (y-j)     * getCellVelocity(i+1, j+1)[index];
}

glm::vec2 SmokeSimulation::getCellVelocity(int i, int j) {
    if (WRAP_BORDERS) {
        if (i < 0) i = GRID_SIZE - 1;
        else if (i >= GRID_SIZE) i = 0;
        if (j < 0) j = GRID_SIZE - 1;
        else if (j >= GRID_SIZE) j = 0;
    } else {
        i = max(i, 0); i = min(i, GRID_SIZE - 1);
        j = max(j, 0); j = min(j, GRID_SIZE - 1);
    }
    return velocityField[i][j];
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            float squareColor[] = { 0.5f, 0.5f, 0.5f, 0.0f };
            setColor(squareColor);
//            drawSquare(transform * translate);

            translate *= glm::translate(glm::vec3(gridSpacing / 2.0f, gridSpacing / 2.0f, 0.0f));

            glm::vec2 velocity = velocityField[i][j];
            glm::mat4 rotate = glm::orientation(glm::vec3(velocity.x+ 0.0000000000001f, velocity.y + 0.0000000000001f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            float velocityColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            setColor(velocityColor);
            drawLine(transform * translate * rotate);
        }
    }

    // Draw interpolated velocity and mouse position
    glm::mat4 translate = glm::translate(glm::vec3(mousePosition, 0.0f));

    glm::vec2 velocity = getVelocity(mousePosition.x - gridSpacing / 2.0f, mousePosition.y - gridSpacing / 2.0f);
    glm::mat4 rotate = glm::orientation(glm::vec3(velocity, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    float mouseColor[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    setColor(mouseColor);
    drawLine(transform * translate * rotate);
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

void SmokeSimulation::drawLine(glm::mat4 transform) {
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
