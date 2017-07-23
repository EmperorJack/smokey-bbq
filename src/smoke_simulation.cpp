#include <iostream>
#include <vector>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

SmokeSimulation::SmokeSimulation() {
    float size = (float) min(SCREEN_WIDTH, SCREEN_HEIGHT);
    gridSpacing = size / GRID_SIZE;
    resetFields();
    setDefaultVariables();
    setDefaultToggles();

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Setup shaders
    simpleShader = loadShaders("resources/shaders/SimpleVertexShader.glsl", "resources/shaders/SimpleFragmentShader.glsl");

    smokeShaders = std::vector<GLuint>();
    smokeShaders.push_back(loadShaders("resources/shaders/SmokeVertexShader.glsl", "resources/shaders/SmokeFragmentShader.glsl"));
    smokeShaders.push_back(loadShaders("resources/shaders/SmokeVertexShader.glsl", "resources/shaders/DensityFragmentShader.glsl"));
    smokeShaders.push_back(loadShaders("resources/shaders/SmokeVertexShader.glsl", "resources/shaders/VelocityFragmentShader.glsl"));
    smokeShaders.push_back(loadShaders("resources/shaders/SmokeVertexShader.glsl", "resources/shaders/TemperatureFragmentShader.glsl"));
    smokeShaders.push_back(loadShaders("resources/shaders/SmokeVertexShader.glsl", "resources/shaders/CurlFragmentShader.glsl"));

    currentShader = 0;
}

void SmokeSimulation::resetFields() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = (i / (float) GRID_SIZE - 0.5f) * 2.0f;
            float y = (j / (float) GRID_SIZE - 0.5f) * 2.0f;
            // velocity[i][j] = glm::vec2(sin(2 * M_PI * y), sin(2 * M_PI * x));
            velocity[i][j] = glm::vec2(0.0f, 0.0f);
            advectedVelocity[i][j] = glm::vec2(0.0f, 0.0f);
            divergence[i][j] = 0.0f;
            pressure[i][j] = 0.0f;
            newPressure[i][j] = 0.0f;
            density[i][j] = 0.0f;
            advectedDensity[i][j] = 0.0f;
            temperature[i][j] = ATMOSPHERE_TEMPERATURE;
            advectedTemperatue[i][j] = ATMOSPHERE_TEMPERATURE;
            tracePosition[i][j] = glm::vec2(0.0f, 0.0f);
            curl[i][j] = 0.0f;
        }
    }
}

void SmokeSimulation::setDefaultVariables() {
    TIME_STEP = 0.1f;
    FLUID_DENSITY = 1.0f;
    JACOBI_ITERATIONS = 25;

    GRAVITY = 0.0981f;
    PULSE_RANGE = 50.0f;
    EMITTER_RANGE = 80.0f;
    PULSE_FORCE = 150.0f;

    VELOCITY_DISSIPATION = 0.98f;
    DENSITY_DISSIPATION = 0.965f;
    TEMPERATURE_DISSIPATION = 0.92f;

    RISE_FORCE = 1.0f;
    FALL_FORCE = 1.0f;
    ATMOSPHERE_TEMPERATURE = 0.0f;

    STROKE_WEIGHT = 2.0f;

    VORTICITY_CONFINEMENT_FORCE = 3.0f;
}

void SmokeSimulation::setDefaultToggles() {
    displayDensityField = true;
    displayVelocityField = false;
    updateSimulation = true;
    enableEmitter = false;
    enablePressureSolve = true;
    randomPulseAngle = false;
    enableBuoyancy = true;
    wrapBorders = false;
    enableVorticityConfinement = true;
}

void SmokeSimulation::update() {
    if (!updateSimulation) return;

    // Advect velocity through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedVelocity[i][j] = getVelocity(tracePosition[i][j].x, tracePosition[i][j].y) * VELOCITY_DISSIPATION;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            velocity[i][j] = advectedVelocity[i][j];
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
                    velocity[i][j] = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);
                    density[i][j] += 0.2f;
                    temperature[i][j] += 1.0f;
                }
            }
        }
    }

    // Buoyancy
    if (enableBuoyancy) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                velocity[i][j] += buoyancyForceAt(i, j);
            }
        }
    }

    // Compute curl
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            curl[i][j] = curlAt(i, j);
        }
    }

    // Vorticity confinement
    if (enableVorticityConfinement) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                velocity[i][j] += vorticityConfinementForceAt(i, j);
            }
        }
    }

    // Compute divergence
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            divergence[i][j] = divergenceAt(i, j);
        }
    }

    // Solve and apply pressure
    solvePressureField();
    if (enablePressureSolve) applyPressure();

    // Compute the trace position
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            tracePosition[i][j] = traceParticle(i * gridSpacing, j * gridSpacing);
        }
    }

    // Advect density and temperature through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedDensity[i][j] = getDensity(tracePosition[i][j].x, tracePosition[i][j].y) * DENSITY_DISSIPATION;
            advectedTemperatue[i][j] = getTemperature(tracePosition[i][j].x, tracePosition[i][j].y) * TEMPERATURE_DISSIPATION;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            density[i][j] = advectedDensity[i][j];
            temperature[i][j] = advectedTemperatue[i][j];
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

    float horizontalSpacing = ((float) SCREEN_WIDTH) / GRID_SIZE;
    float verticalSpacing = ((float) SCREEN_HEIGHT) / GRID_SIZE;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * horizontalSpacing;
            float y = j * verticalSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < PULSE_RANGE) {
                float falloffFactor = (1 - glm::distance(position, gridPosition) / PULSE_RANGE);

                velocity[i][j] = PULSE_FORCE * (randomPulseAngle ? glm::normalize(force) : glm::normalize(gridPosition - position));
                density[i][j] += 0.5f * falloffFactor;
                temperature[i][j] += 2.5f * falloffFactor;
            }
        }
    }
}

void SmokeSimulation::emit(glm::vec2 position, glm::vec2 force, float range, float densityAmount, float temperatureAmount) {
    float horizontalSpacing = ((float) SCREEN_WIDTH) / GRID_SIZE;
    float verticalSpacing = ((float) SCREEN_HEIGHT) / GRID_SIZE;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * horizontalSpacing;
            float y = j * verticalSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < range) {
                float falloffFactor = (1 - glm::distance(position, gridPosition) / range);

                velocity[i][j] = force;
                density[i][j] += densityAmount * falloffFactor;
                temperature[i][j] += temperatureAmount * falloffFactor;
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

glm::vec2 SmokeSimulation::buoyancyForceAt(int i, int j) {
    return (FALL_FORCE * density[i][j] - RISE_FORCE * (temperature[i][j] - ATMOSPHERE_TEMPERATURE)) * glm::vec2(0.0f, GRAVITY / abs(GRAVITY));
}

float SmokeSimulation::curlAt(int i, int j) {
    float pdx = (getInterpolatedVelocity(i + 1, j, false) -
                 getInterpolatedVelocity(i - 1, j, false)) * 0.5f;
    float pdy = (getInterpolatedVelocity(i, j + 1, true) -
                 getInterpolatedVelocity(i, j - 1, true)) * 0.5f;

    return pdx - pdy;
}

glm::vec2 SmokeSimulation::vorticityConfinementForceAt(int i, int j) {
    float curl = getGridCurl(i, j);
    float curlLeft = getGridCurl(i - 1, j);
    float curlRight = getGridCurl(i + 1, j);
    float curlBottom = getGridCurl(i, j - 1);
    float curlTop = getGridCurl(i, j + 1);

    glm::vec3 magnitude = glm::vec3(abs(curlRight) - abs(curlLeft), abs(curlTop) - abs(curlBottom), 0.0f);

    float length = glm::length(magnitude);
    magnitude = length > 0.0f ? (magnitude / length) : glm::vec3();

    glm::vec3 curlVector = glm::vec3(0.0f, 0.0f, curl);
    glm::vec3 force = TIME_STEP * VORTICITY_CONFINEMENT_FORCE * glm::cross(magnitude, curlVector);

    return glm::vec2(force);
}

void SmokeSimulation::solvePressureField() {
    // Reset the pressure field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            pressure[i][j] = 0.0f;
        }
    }

    // Iteratively solve the new pressure field
    for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                newPressure[i][j] = pressureAt(i, j);
            }
        }

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                pressure[i][j] = newPressure[i][j];
            }
        }
    }
}

float SmokeSimulation::pressureAt(int i, int j) {
    float d = divergence[i][j];
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
            float xChange = getGridPressure(clampIndex(i + 1), j) - getGridPressure(clampIndex(i - 1), j);
            float yChange = getGridPressure(i, clampIndex(j + 1)) - getGridPressure(i,clampIndex(j - 1));

            velocity[i][j].x += a * xChange;
            velocity[i][j].y += a * yChange;
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
        return velocity[wrapIndex(i)][wrapIndex(j)];
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return velocity[i][j] * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridDensity(int i, int j) {
    if (wrapBorders) {
        return density[wrapIndex(i)][wrapIndex(j)];
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return density[i][j] * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridTemperature(int i, int j) {
    if (wrapBorders) {
        return temperature[wrapIndex(i)][wrapIndex(j)];
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return temperature[i][j] * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridPressure(int i, int j) {
    if (wrapBorders) {
        return pressure[wrapIndex(i)][wrapIndex(j)];
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return pressure[i][j] * (boundary ? 0.0f : 1.0f);
    }
}

float SmokeSimulation::getGridCurl(int i, int j) {
    if (wrapBorders) {
        return curl[wrapIndex(i)][wrapIndex(j)];
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return curl[i][j] * (boundary ? 0.0f : 1.0f);
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
    if (displayDensityField) renderField();
    if (displayVelocityField) renderVelocityField(transform, mousePosition);
}

void SmokeSimulation::renderField() {
    glUseProgram(smokeShaders[currentShader]);

    passScreenSize(smokeShaders[currentShader]);

    float textureField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE][2];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            switch (currentShader) {
                case 0:
                    textureField[i][j][0] = density[j][i];
                    textureField[i][j][1] = temperature[j][i];
                    break;
                case 1:
                    textureField[i][j][0] = density[j][i];
                    break;
                case 2:
                    textureField[i][j][0] = velocity[j][i].x / 10.0f;
                    textureField[i][j][1] = velocity[j][i].y / 10.0f;
                    break;
                case 3:
                    textureField[i][j][0] = temperature[j][i];
                    break;
                case 4:
                    textureField[i][j][0] = curl[j][i];
                    break;
                default:
                    textureField[i][j][0] = 0.0f;
                    textureField[i][j][1] = 0.0f;
                    break;
            }
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &textureField[0][0][0]);

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

void SmokeSimulation::renderVelocityField(glm::mat4 transform, glm::vec2 mousePosition) {
    glUseProgram(simpleShader);

    float velocityColor[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(simpleShader, velocityColor);

    float horizontalSpacing = ((float) SCREEN_WIDTH) / GRID_SIZE;
    float verticalSpacing = ((float) SCREEN_HEIGHT) / GRID_SIZE;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * horizontalSpacing;
            float y = j * verticalSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            translate *= glm::translate(glm::vec3(horizontalSpacing / 2.0f, verticalSpacing / 2.0f, 0.0f));

            glm::vec3 velocityAmount = glm::vec3(velocity[i][j].x, velocity[i][j].y, 0.0f);

            float magnitude = max(min(glm::length(velocityAmount) / PULSE_FORCE * 1.5f, 2.0f), 0.5f);

            glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
            glm::mat4 rotate = glm::orientation(glm::normalize(velocityAmount), glm::vec3(0.0f, 1.0f, 0.0f));

            drawLine(transform * translate * scale * rotate);
        }
    }

    // Draw interpolated velocity and mouse position
    glm::mat4 translate = glm::translate(glm::vec3(mousePosition, 0.0f));

    glm::vec2 velocityAmount = getVelocity(mousePosition.x - horizontalSpacing / 2.0f, mousePosition.y - verticalSpacing / 2.0f);

    float magnitude = min(glm::length(velocityAmount), 2.0f);
    glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
    glm::mat4 rotate = glm::orientation(glm::vec3(glm::normalize(velocityAmount), 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    float mouseColor[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    setColor(simpleShader, mouseColor);
    drawLine(transform * translate * scale * rotate);
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
