#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <iostream>

bool displayVectors = false;
bool displayDensity = true;
bool enableEmitter = false;
bool enablePressureSolve = true;

glm::vec2 velocityField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
float divergenceField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
float pressureField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
glm::vec2 densityField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];

GLuint squareVBO;
GLuint velocityVBO;
GLuint densityVBO;
GLuint densityUVVBO;
GLuint densityTextureID;

float gridWorldSize;
float gridSpacing;

SmokeSimulation::SmokeSimulation(float _gridWorldSize) {
    gridWorldSize = _gridWorldSize;
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

    float densityVertices[] = {
            -gridWorldSize / 2.0f, -gridWorldSize / 2.0f,
            -gridWorldSize / 2.0f, gridWorldSize / 2.0f,
            gridWorldSize / 2.0f, gridWorldSize / 2.0f,
            gridWorldSize / 2.0f, -gridWorldSize / 2.0f,
    };
    glGenBuffers(1, &densityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(densityVertices), densityVertices, GL_STATIC_DRAW);

    float densityUVs[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
    };
    glGenBuffers(1, &densityUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, densityUVVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(densityUVs), densityUVs, GL_STATIC_DRAW);

    // Setup density texture
    glGenTextures(1, &densityTextureID);

    glBindTexture(GL_TEXTURE_2D, densityTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void SmokeSimulation::setupFields() {
    // Setup velocity field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing + gridSpacing / 2.0f;
            float y = j * gridSpacing + gridSpacing / 2.0f;
            velocityField[i][j] =
                    //glm::normalize
            (glm::vec2(
                    //sin(2 * M_PI * y), sin(2 * M_PI * x)
                    //y, x
                    //-0.01f, -0.01f
                    0.0f, 0.0f
                    //myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f
                    //1.0f, sin(2 * M_PI * y)
            ));
        }
    }

    // Setup density field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            densityField[i][j] = glm::vec2(0.0f, 0.0f);
        }
    }
}

void SmokeSimulation::update() {
    glm::vec2 advectedVelocityField[GRID_SIZE][GRID_SIZE];
    glm::vec2 advectedDensityField[GRID_SIZE][GRID_SIZE];

    // Advect velocity through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedVelocityField[i][j] = advectedVelocityAt(i, j);
        }
    }

    std::copy(&advectedVelocityField[0][0], &advectedVelocityField[0][0] + GRID_SIZE * GRID_SIZE, &velocityField[0][0]);

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 target = glm::vec2(GRID_SIZE / 2 * gridSpacing, GRID_SIZE * gridSpacing - 2);

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                float x = i * gridSpacing;
                float y = j * gridSpacing;
                glm::vec2 gridPosition = glm::vec2(x, y);
                if (glm::distance(target, gridPosition) < PULSE_RANGE / 3.0f) {
                    float horizontalForce = PULSE_FORCE * 50;
                    velocityField[i][j] = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);
                    densityField[i][j] += 0.2f;
                }
            }
        }
    }

    // Compute divergence
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            divergenceField[i][j] = divergenceAt(i, j);
        }
    }

    // Solve and apply pressure
    solvePressureField();
    if (enablePressureSolve) applyPressure();

    // Advect density through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedDensityField[i][j] = glm::vec2(advectedDensityAt(i, j) * DENSITY_DISSAPATION, 0.0f);
        }
    }

    std::copy(&advectedDensityField[0][0], &advectedDensityField[0][0] + GRID_SIZE * GRID_SIZE, &densityField[0][0]);
}

glm::vec2 SmokeSimulation::advectedVelocityAt(int i, int j) {
    glm::vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing);

    return getVelocity(tracePosition.x, tracePosition.y);
}

float SmokeSimulation::advectedDensityAt(int i, int j) {
    glm::vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing);

    return getDensity(tracePosition.x, tracePosition.y);
}

glm::vec2 SmokeSimulation::traceParticle(float x, float y) {
    //return glm::vec2(x, y) - (TIME_STEP * getVelocity(x, y));
    glm::vec2 v = getVelocity(x, y);
    v = getVelocity(x + 0.5f * TIME_STEP * v.x, y + 0.5f * TIME_STEP * v.y);
    return glm::vec2(x, y) - (TIME_STEP * v);

    // Euler method:         y = x + ∆t * u(x)
    // Runge kutta 2 method: y = x + ∆t * u(x + ∆t / 2 * u(x))
}

float SmokeSimulation::divergenceAt(int i, int j) {
    float a = -((2 * gridSpacing * FLUID_DENSITY) / TIME_STEP);

    float b = getVelocity((i + 1) * gridSpacing, j * gridSpacing).x -
              getVelocity((i - 1) * gridSpacing, j * gridSpacing).x +
              getVelocity(i * gridSpacing, (j + 1) * gridSpacing).y -
              getVelocity(i * gridSpacing, (j - 1) * gridSpacing).y;

    return a * b;
}

void SmokeSimulation::solvePressureField() {
    // Reset the pressure field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            pressureField[i][j] = 0.0f;
        }
    }

    // Iteratively solve the new pressure field
    for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
        float nextPressureField[GRID_SIZE][GRID_SIZE];

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                nextPressureField[i][j] = pressureAt(i, j);
            }
        }

        std::copy(&nextPressureField[0][0], &nextPressureField[0][0] + GRID_SIZE * GRID_SIZE, &pressureField[0][0]);
    }
}

float SmokeSimulation::pressureAt(int i, int j) {
    float d = divergenceField[i][j];
    float p = pressureField[iClamp(i + 2)][j] +
              pressureField[iClamp(i - 2)][j] +
              pressureField[i][jClamp(j + 2)] +
              pressureField[i][jClamp(j - 2)];
    return (d + p) / 4.0f;
}

void SmokeSimulation::applyPressure() {
    float a = -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing));

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {

            float xChange = pressureField[iClamp(i + 1)][j] - pressureField[iClamp(i - 1)][j];
            float yChange = pressureField[i][jClamp(j + 1)] - pressureField[i][jClamp(j - 1)];

            velocityField[i][j].x += a * xChange;
            velocityField[i][j].y += a * yChange;
        }
    }
}

glm::vec2 SmokeSimulation::getVelocity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    glm::vec2 v = glm::vec2();

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedValue(velocityField, normX - 0.5f, normY, 0) +
           getInterpolatedValue(velocityField, normX + 0.5f, normY, 0)) / 2.0f;
    v.y = (getInterpolatedValue(velocityField, normX, normY - 0.5f, 1) +
           getInterpolatedValue(velocityField, normX, normY + 0.5f, 1)) / 2.0f;

    //if (v.x == 0.0f && v.y == 0.0f) std::cout << "YO NAN " << std::endl;

    return v;
}

float SmokeSimulation::getDensity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedValue(densityField, normX, normY, 0);
}

float SmokeSimulation::getInterpolatedValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], float x, float y, int index) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getGridValue(field, i, j)[index] +
           (x-i) * (j+1-y)   * getGridValue(field, i+1, j)[index] +
           (i+1-x) * (y-j)   * getGridValue(field, i, j+1)[index] +
           (x-i) * (y-j)     * getGridValue(field, i+1, j+1)[index];
}

glm::vec2 SmokeSimulation::getGridValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], int i, int j) {
    return field[iClamp(i)][jClamp(j)];
}

int SmokeSimulation::iClamp(int i) {
    if (WRAP_BORDERS) {
        if (i < 0) i = GRID_SIZE + (i % GRID_SIZE);
        else i = i % GRID_SIZE;
    } else {
        i = max(i, 0); i = min(i, GRID_SIZE - 1);
    }

    return i;
}

int SmokeSimulation::jClamp(int j) {
    if (WRAP_BORDERS) {
        if (j < 0) j = GRID_SIZE + (j % GRID_SIZE);
        else j = j % GRID_SIZE;
    } else {
        j = max(j, 0); j = min(j, GRID_SIZE - 1);
    }
    if (j < 0) std::cout << "WHAT THE FUCK" << std::endl;
    return j;
}

void SmokeSimulation::addPulse(glm::vec2 position) {
    position -= glm::vec2(gridSpacing / 2.0f, gridSpacing / 2.0f);

    glm::vec2 force = glm::vec2(myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < PULSE_RANGE) {
                velocityField[i][j] = PULSE_FORCE * glm::normalize(glm::vec2(force)); // gridPosition - position // 0.0f, -1.0f
                densityField[i][j] += 0.5f * (1 - glm::distance(position, gridPosition) / PULSE_RANGE);
            }
        }
    }
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    if (displayDensity) {
        glm::mat4 densityTransform = glm::mat4(transform);
        densityTransform = glm::translate(densityTransform, glm::vec3(gridWorldSize / 2.0f, gridWorldSize / 2.0f, 0.0f));
        densityTransform = glm::scale(densityTransform, glm::vec3(-1.0f, 1.0f, 1.0f));
        densityTransform = glm::rotate(densityTransform, (float) M_PI / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        drawDensity(densityTransform);
    }

    if (!displayVectors) return;

    float velocityColor[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(velocityColor);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            translate *= glm::translate(glm::vec3(gridSpacing / 2.0f, gridSpacing / 2.0f, 0.0f));

            glm::vec3 velocity = glm::vec3(velocityField[i][j].x, velocityField[i][j].y, 0.0f);

            float magnitude = max(min(glm::length(velocity) / PULSE_FORCE * 1.5f, 2.0f), 0.5f);

            glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
            glm::mat4 rotate = glm::orientation(glm::normalize(velocity), glm::vec3(0.0f, 1.0f, 0.0f));

            drawLine(transform * translate * scale * rotate);
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

void SmokeSimulation::drawDensity(glm::mat4 transform) {
    GLint useFillColorLocation = glGetUniformLocation(3, "useFillColor");
    glUniform1i(useFillColorLocation, false);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &densityField[0][0]);

    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    // Bind vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    // Bind uvs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, densityUVVBO);
    glVertexAttribPointer(
            1,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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

void SmokeSimulation::toggleVectorDisplay() {
    displayVectors = !displayVectors;
}

void SmokeSimulation::toggleDensityDisplay() {
    displayDensity = !displayDensity;
}

void SmokeSimulation::toggleEnableEmitter() {
    enableEmitter = !enableEmitter;
}

void SmokeSimulation::togglePressureSolve() {
    enablePressureSolve = !enablePressureSolve;
}
