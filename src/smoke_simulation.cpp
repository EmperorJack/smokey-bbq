#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <iostream>

const int GRID_SIZE = 50;
const float TIME_STEP = 10.0f;
const bool WRAP_BORDERS = true;
const float STROKE_WEIGHT = 2.0f;
const float PULSE_RANGE = 200.0f;

glm::vec2 velocityField[GRID_SIZE][GRID_SIZE];
float densityField[GRID_SIZE][GRID_SIZE];
GLuint squareVBO;
GLuint velocityVBO;

float gridSpacing;

SmokeSimulation::SmokeSimulation(float gridWorldSize) {
    gridSpacing = gridWorldSize / GRID_SIZE;
    setupFields();

    // Setup VBOs
    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, gridSpacing,
            gridSpacing, gridSpacing,
            gridSpacing, 0.0f,
    };
    glGenBuffers(1, &squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    float vectorVertices[] = {
            -STROKE_WEIGHT / 2.0f, 0.0f,
            -STROKE_WEIGHT / 2.0f, gridSpacing / 2.5f,
            STROKE_WEIGHT / 2.0f, gridSpacing / 2.5f,
            STROKE_WEIGHT / 2.0f, 0.0f,
    };
    glGenBuffers(1, &velocityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vectorVertices), vectorVertices, GL_STATIC_DRAW);
}

void SmokeSimulation::setupFields() {
    // Setup velocity field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing + gridSpacing / 2.0f;
            float y = j * gridSpacing + gridSpacing / 2.0f;
            velocityField[i][j] = glm::normalize(glm::vec2(
                    //sin(2 * M_PI * y), sin(2 * M_PI * x)
                    y, x
                    //myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f
                    //0.01f, 0.01f
                    //1.0f, sin(2 * M_PI * y)
            ));
        }
    }

    // Setup density field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            densityField[i][j] = myRandom();//0.0f;
        }
    }
}

void SmokeSimulation::update() {
    glm::vec2 advectedVelocityField[GRID_SIZE][GRID_SIZE];
    float advectedDensityField[GRID_SIZE][GRID_SIZE];

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedVelocityField[i][j] = advectedVelocityAt(i, j);
            advectedDensityField[i][j] = advectedDensityAt(i, j) * 0.985f;
            //advectedVelocityField[i][j] = glm::rotate(velocityField[i][j], (float) M_PI / 120.0f );
        }
    }

    std::copy(&advectedVelocityField[0][0], &advectedVelocityField[0][0] + GRID_SIZE * GRID_SIZE, &velocityField[0][0]);
    std::copy(&advectedDensityField[0][0], &advectedDensityField[0][0] + GRID_SIZE * GRID_SIZE, &densityField[0][0]);
}

glm::vec2 SmokeSimulation::advectedVelocityAt(int i, int j) {
    glm::vec2 cellPosition = glm::vec2(i * gridSpacing, j * gridSpacing);
    glm::vec2 tracePosition = traceParticle(cellPosition.x, cellPosition.y, TIME_STEP);

    return getVelocity(tracePosition.x, tracePosition.y);
}

float SmokeSimulation::advectedDensityAt(int i, int j) {
    glm::vec2 cellPosition = glm::vec2(i * gridSpacing, j * gridSpacing);
    glm::vec2 tracePosition = traceParticle(cellPosition.x, cellPosition.y, TIME_STEP);

    return getDensity(tracePosition.x, tracePosition.y);
}


glm::vec2 SmokeSimulation::traceParticle(float x, float y, float timeStep) {
    //return glm::vec2(x, y) - (timeStep * getVelocity(x, y));
    glm::vec2 v = getVelocity(x, y);
    v = getVelocity(x + 0.5f * timeStep * v.x, y + 0.5f * timeStep * v.y);
    return glm::vec2(x, y) - (timeStep * v);

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

float SmokeSimulation::getDensity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedValueDensity(normX, normY);
}

float SmokeSimulation::getInterpolatedValue(float x, float y, int index) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getCellVelocity(i, j)[index] +
           (x-i) * (j+1-y)   * getCellVelocity(i+1, j)[index] +
           (i+1-x) * (y-j)   * getCellVelocity(i, j+1)[index] +
           (x-i) * (y-j)     * getCellVelocity(i+1, j+1)[index];
}

float SmokeSimulation::getInterpolatedValueDensity(float x, float y) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getCellDensity(i, j) +
           (x-i) * (j+1-y)   * getCellDensity(i+1, j) +
           (i+1-x) * (y-j)   * getCellDensity(i, j+1) +
           (x-i) * (y-j)     * getCellDensity(i+1, j+1);
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

float SmokeSimulation::getCellDensity(int i, int j) {
    if (WRAP_BORDERS) {
        if (i < 0) i = GRID_SIZE - 1;
        else if (i >= GRID_SIZE) i = 0;
        if (j < 0) j = GRID_SIZE - 1;
        else if (j >= GRID_SIZE) j = 0;
    } else {
        i = max(i, 0); i = min(i, GRID_SIZE - 1);
        j = max(j, 0); j = min(j, GRID_SIZE - 1);
    }
    return densityField[i][j];
}

void SmokeSimulation::addPulse(glm::vec2 position) {
    position -= glm::vec2(gridSpacing / 2.0f, gridSpacing / 2.0f);

    glm::vec2 force = glm::vec2(myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f); // gridPosition - position

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < PULSE_RANGE) {
                velocityField[i][j] =  15.0f * glm::normalize(glm::vec2(force)); // gridPosition - position // 0.0f, -1.0f
                densityField[i][j] += 0.5f / (glm::distance(position, gridPosition) / PULSE_RANGE);
            }
        }
    }
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            float density = densityField[i][j];
            if (density > 0.01f) {
                float squareColor[] = {density, density, density, 0.0f};
                setColor(squareColor);
                drawSquare(transform * translate, true);
            }

            translate *= glm::translate(glm::vec3(gridSpacing / 2.0f, gridSpacing / 2.0f, 0.0f));

            glm::vec3 velocity = glm::vec3(velocityField[i][j].x, velocityField[i][j].y, 0.0f);

            float magnitude = min(glm::length(velocity), 2.0f);

            if (i == 0 && j == 0) {
                //std::cout << velocity.x << ", " << velocity.y << " : " << magnitude << std::endl;
            }

            glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
            glm::mat4 rotate = glm::orientation(glm::normalize(velocity), glm::vec3(0.0f, 1.0f, 0.0f));

            float velocityColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
            setColor(velocityColor);
            //drawLine(transform * translate * scale * rotate);
        }
    }

    // Draw interpolated velocity and mouse position
    glm::mat4 translate = glm::translate(glm::vec3(mousePosition, 0.0f));

    glm::vec2 velocity = getVelocity(mousePosition.x - gridSpacing / 2.0f, mousePosition.y - gridSpacing / 2.0f);

    float magnitude = min(glm::length(velocity), 2.0f);
    glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
    glm::mat4 rotate = glm::orientation(glm::vec3(glm::normalize(velocity), 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    float mouseColor[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    setColor(mouseColor);
    drawLine(transform * translate * scale * rotate);
}

void SmokeSimulation::drawSquare(glm::mat4 transform, bool fill) {
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

    if (fill) {
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else {
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
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

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(0);
}

float SmokeSimulation::myRandom() {
    return std::rand() % 100 / 100.0f;
}
