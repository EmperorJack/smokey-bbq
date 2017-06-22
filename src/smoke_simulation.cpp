#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

SmokeSimulation::SmokeSimulation() {
    gridSpacing = ((float) SCREEN_WIDTH) / GRID_SIZE;
    resetFields();

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
        -1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, 3.0, 0.0, 1.0,
        3.0, -1.0, 0.0, 1.0
    };
    glGenBuffers(1, &densityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(densityVertices), densityVertices, GL_STATIC_DRAW);

    // Setup density texture
    glGenTextures(1, &densityTexture);

    glBindTexture(GL_TEXTURE_2D, densityTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Setup shaders
    simpleShader = loadShaders("resources/shaders/SimpleVertexShader.glsl", "resources/shaders/SimpleFragmentShader.glsl");
    densityShader = loadShaders("resources/shaders/DensityVertexShader.glsl", "resources/shaders/DensityFragmentShader.glsl");
}

void SmokeSimulation::resetFields() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            gridCell g;
            g.velocity = glm::vec2(0.0f, 0.0f);
            g.advectedVelocity = glm::vec2(0.0f, 0.0f);
            g.divergence = 0.0f;
            g.pressure = 0.0f;
            g.newPressure = 0.0f;
            g.density = 0.0f;
            g.advectedDensity = 0.0f;
            g.temperature = ATMOSPHERE_TEMPERATURE;
            g.advectedTemperatue = ATMOSPHERE_TEMPERATURE;
            g.tracePosition = glm::vec2(0.0f, 0.0f);
            grid[i][j] = g;
        }
    }
}

void SmokeSimulation::update() {

    // Advect velocity through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].advectedVelocity = getVelocity(grid[i][j].tracePosition.x, grid[i][j].tracePosition.y) * VELOCITY_DISSAPATION;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].velocity = grid[i][j].advectedVelocity;
        }
    }

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 target = glm::vec2(GRID_SIZE / 2 * gridSpacing, GRID_SIZE * gridSpacing - 2);

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                float x = i * gridSpacing;
                float y = j * gridSpacing;
                glm::vec2 gridPosition = glm::vec2(x, y);
                if (glm::distance(target, gridPosition) < EMITTER_RANGE) {
                    float horizontalForce = PULSE_FORCE * 50;
                    grid[i][j].velocity = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);
                    grid[i][j].density += 0.2f;
                    grid[i][j].temperature += 1.0f;
                }
            }
        }
    }

    // Buoyancy
    if (enableBuoyancy) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                grid[i][j].velocity += buoyancyAt(i, j);
            }
        }
    }

    // Compute divergence
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].divergence = divergenceAt(i, j);
        }
    }

    // Solve and apply pressure
    solvePressureField();
    if (enablePressureSolve) applyPressure();

    // Compute the trace position
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].tracePosition = traceParticle(i * gridSpacing, j * gridSpacing);
        }
    }

    // Advect density and temperature through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].advectedDensity = getDensity(grid[i][j].tracePosition.x, grid[i][j].tracePosition.y) * DENSITY_DISSAPATION;
            grid[i][j].advectedTemperatue = getTemperature(grid[i][j].tracePosition.x, grid[i][j].tracePosition.y) * TEMPERATURE_DISSAPATION;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].density = grid[i][j].advectedDensity;
            grid[i][j].temperature = grid[i][j].advectedTemperatue;
        }
    }
}

void SmokeSimulation::addPulse(glm::vec2 position) {
    position -= glm::vec2(gridSpacing / 2.0f, gridSpacing / 2.0f);

    glm::vec2 force = glm::vec2(0.0f, 0.0f);
    if (randomPulseAngle) {
        while (force.x == 0.0f && force.y == 0.0f) {
            force = glm::vec2(myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f);
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < PULSE_RANGE) {
                grid[i][j].velocity = PULSE_FORCE * (randomPulseAngle ? glm::normalize(force) : glm::normalize(gridPosition - position));
                grid[i][j].density += 0.5f * (1 - glm::distance(position, gridPosition) / PULSE_RANGE);
                grid[i][j].temperature += 2.5f * (1 - glm::distance(position, gridPosition) / PULSE_RANGE);
            }
        }
    }
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

glm::vec2 SmokeSimulation::buoyancyAt(int i, int j) {
    return (FALL_FORCE * grid[i][j].density - RISE_FORCE * (grid[i][j].temperature - ATMOSPHERE_TEMPERATURE)) * glm::vec2(0.0f, GRAVITY / abs(GRAVITY));
}

void SmokeSimulation::solvePressureField() {
    // Reset the pressure field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].pressure = 0.0f;
        }
    }

    // Iteratively solve the new pressure field
    for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                grid[i][j].newPressure = pressureAt(i, j);
            }
        }

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                grid[i][j].pressure = grid[i][j].newPressure;
            }
        }
    }
}

float SmokeSimulation::pressureAt(int i, int j) {
    float d = grid[i][j].divergence;
    float p = getGridPressure(clampIndex(i + 2), j) +
              getGridPressure(clampIndex(i - 2), j) +
              getGridPressure(i, clampIndex(j + 2)) +
              getGridPressure(i, clampIndex(j - 2));
    return (d + p) * 0.25f;
}

void SmokeSimulation::applyPressure() {
    float a = -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing));

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float xChange = getGridPressure(clampIndex(i + 1),j) - getGridPressure(clampIndex(i - 1),j);
            float yChange = getGridPressure(i, clampIndex(j + 1)) - getGridPressure(i ,clampIndex(j - 1));

            grid[i][j].velocity.x += a * xChange;
            grid[i][j].velocity.y += a * yChange;
        }
    }
}

glm::vec2 SmokeSimulation::getVelocity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    glm::vec2 v = glm::vec2();

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedVelocity(normX - 0.5f, normY, true) +
           getInterpolatedVelocity(normX + 0.5f, normY, true)) * 0.5f;
    v.y = (getInterpolatedVelocity(normX, normY - 0.5f, false) +
           getInterpolatedVelocity(normX, normY + 0.5f, false)) * 0.5f;

    //if (v.x == 0.0f && v.y == 0.0f) std::cout << "YO NAN " << std::endl;

    return v;
}

float SmokeSimulation::getDensity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedDensity(normX, normY);
}

float SmokeSimulation::getTemperature(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedTemperature(normX, normY);
}

float SmokeSimulation::getInterpolatedVelocity(float x, float y, bool xAxis) {
    int i = ((int) (x + GRID_SIZE)) - GRID_SIZE;
    int j = ((int) (y + GRID_SIZE)) - GRID_SIZE;

    return (i+1-x) * (j+1-y) * (xAxis ? getGridVelocity(i, j).x : getGridVelocity(i, j).y) +
           (x-i) * (j+1-y)   * (xAxis ? getGridVelocity(i+1, j).x : getGridVelocity(i+1, j).y) +
           (i+1-x) * (y-j)   * (xAxis ? getGridVelocity(i, j+1).x : getGridVelocity(i, j+1).y) +
           (x-i) * (y-j)     * (xAxis ? getGridVelocity(i+1, j+1).x : getGridVelocity(i+1, j+1).y);
}

float SmokeSimulation::getInterpolatedDensity(float x, float y) {
    int i = ((int) (x + GRID_SIZE)) - GRID_SIZE;
    int j = ((int) (y + GRID_SIZE)) - GRID_SIZE;

    return (i+1-x) * (j+1-y) * getGridDensity(i, j) +
           (x-i) * (j+1-y)   * getGridDensity(i+1, j) +
           (i+1-x) * (y-j)   * getGridDensity(i, j+1) +
           (x-i) * (y-j)     * getGridDensity(i+1, j+1);
}

float SmokeSimulation::getInterpolatedTemperature(float x, float y) {
    int i = ((int) (x + GRID_SIZE)) - GRID_SIZE;
    int j = ((int) (y + GRID_SIZE)) - GRID_SIZE;

    return (i+1-x) * (j+1-y) * getGridTemperature(i, j) +
           (x-i) * (j+1-y)   * getGridTemperature(i+1, j) +
           (i+1-x) * (y-j)   * getGridTemperature(i, j+1) +
           (x-i) * (y-j)     * getGridTemperature(i+1, j+1);
}

glm::vec2 SmokeSimulation::getGridVelocity(int i, int j) {
    if (wrapBorders) {
        return grid[wrapIndex(i)][wrapIndex(j)].velocity;
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return grid[i][j].velocity * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridDensity(int i, int j) {
    if (wrapBorders) {
        return grid[wrapIndex(i)][wrapIndex(j)].density;
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return grid[i][j].density * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridTemperature(int i, int j) {
    if (wrapBorders) {
        return grid[wrapIndex(i)][wrapIndex(j)].temperature;
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return grid[i][j].temperature * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridPressure(int i, int j) {
    if (wrapBorders) {
        return grid[wrapIndex(i)][wrapIndex(j)].pressure;
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return grid[i][j].pressure * (boundary ? 0.0f : 1.0f);
    }
}

int SmokeSimulation::wrapIndex(int i) {
    if (i < 0) i = GRID_SIZE + (i % GRID_SIZE);
    else i = i >= GRID_SIZE ? i % GRID_SIZE : i;

    return i;
}

int SmokeSimulation::clampIndex(int i) {
    if (i < 0) i = 0;
    else if (i >= GRID_SIZE) i = GRID_SIZE - 1;

    return i;
}

bool SmokeSimulation::clampBoundary(int &i) {
    if (i < 0) {
        i = 0;
        return true;
    }  else if (i >= GRID_SIZE) {
        i = GRID_SIZE - 1;
        return true;
    }

    return false;
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    if (displayDensity) drawDensity();

    if (!displayVectors) return;

    glUseProgram(simpleShader);

    float velocityColor[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(velocityColor);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            translate *= glm::translate(glm::vec3(gridSpacing / 2.0f, gridSpacing / 2.0f, 0.0f));

            glm::vec3 velocity = glm::vec3(grid[i][j].velocity.x, grid[i][j].velocity.y, 0.0f);

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

void SmokeSimulation::drawDensity() {
    glUseProgram(densityShader);

    float densityField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE][2];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            densityField[i][j][0] = grid[j][i].density; // grid[j][i].pressure / 1000.0f;
            densityField[i][j][1] = grid[j][i].temperature; // grid[j][i].divergence / 1000.0f;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &densityField[0][0][0]);

    // Bind vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            4,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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

void SmokeSimulation::togglePulseType() {
    randomPulseAngle = !randomPulseAngle;
}

void SmokeSimulation::toggleBuoyancy() {
    enableBuoyancy = !enableBuoyancy;
}

void SmokeSimulation::toggleWrapBorders() {
    wrapBorders = !wrapBorders;
}