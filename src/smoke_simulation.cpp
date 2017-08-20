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
        -1.0f, 3.0f, 0.0f, 1.0f,
        3.0f, -1.0f, 0.0f, 1.0f
    };
    glGenBuffers(1, &fullscreenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(densityVertices), densityVertices, GL_STATIC_DRAW);

    // Setup textures for rendering fields
    glGenTextures(1, &textureA);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &textureB);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Setup shaders
    simpleShader = loadShaders("SimpleVertexShader", "SimpleFragmentShader");

    smokeShaders = std::vector<GLuint>();
    smokeShaders.push_back(loadShaders("SmokeVertexShader", "SmokeFragmentShader"));
    smokeShaders.push_back(loadShaders("SmokeVertexShader", "DensityFragmentShader"));
    smokeShaders.push_back(loadShaders("SmokeVertexShader", "VelocityFragmentShader"));
    smokeShaders.push_back(loadShaders("SmokeVertexShader", "TemperatureFragmentShader"));
    smokeShaders.push_back(loadShaders("SmokeVertexShader", "CurlFragmentShader"));

    currentShader = 0;

    init();
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

    clearSlabs();
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
    enableEmitter = true;
    enablePressureSolve = true;
    randomPulseAngle = false;
    enableBuoyancy = false;
    wrapBorders = false;
    enableVorticityConfinement = false;
    gpuImplementation = true;
}

void SmokeSimulation::update() {
    if (!updateSimulation) return;

    glViewport(0, 0, GRID_SIZE, GRID_SIZE);

    // Advect velocity through velocity
    if (gpuImplementation) {
        //loadVectorFieldIntoTexture(velocitySlab.ping.textureHandle, velocity);

        advect(velocitySlab.ping, velocitySlab.ping, velocitySlab.pong, VELOCITY_DISSIPATION);
        //copyVectorTextureIntoField(velocitySlab.pong.textureHandle, velocity);
        swapSurfaces(velocitySlab);
        resetState();
    } else {
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
    }

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 target = glm::vec2(GRID_SIZE / 2 * gridSpacing, GRID_SIZE * gridSpacing - 2);
        glm::vec2 force = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);

        if (gpuImplementation) {
            swapSurfaces(velocitySlab);
            applyImpulse(velocitySlab.pong, target / gridSpacing, EMITTER_RANGE / gridSpacing, glm::vec3(force / 5.0f, 0.0f));
            //copyVectorTextureIntoField(velocitySlab.pong.textureHandle, velocity);
            swapSurfaces(velocitySlab);
            resetState();

            swapSurfaces(densitySlab);
            applyImpulse(densitySlab.pong, target / gridSpacing, EMITTER_RANGE / gridSpacing, glm::vec3(1.0f, 0.0f, 0.0f));
            //copyScalarTextureIntoField(densitySlab.pong.textureHandle, density);
            swapSurfaces(densitySlab);
            resetState();

            swapSurfaces(temperatureSlab);
            applyImpulse(temperatureSlab.pong, target / gridSpacing, EMITTER_RANGE / gridSpacing, glm::vec3(5.0f, 0.0f, 0.0f));
            //copyScalarTextureIntoField(temperatureSlab.pong.textureHandle, temperature);
            swapSurfaces(temperatureSlab);
            resetState();
        } else {
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    float x = i * gridSpacing;
                    float y = j * gridSpacing;
                    glm::vec2 gridPosition = glm::vec2(x, y);
                    if (glm::distance(target, gridPosition) < EMITTER_RANGE) {
                        velocity[i][j] = force;
                        density[i][j] += 0.2f;
                        temperature[i][j] += 1.0f;
                    }
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
    if (gpuImplementation) {
        //loadVectorFieldIntoTexture(velocitySlab.ping.textureHandle, velocity);
        computeDivergence(velocitySlab.ping, divergenceSlab.pong);
        //copyScalarTextureIntoField(divergenceSlab.pong.textureHandle, divergence);
        swapSurfaces(divergenceSlab);
        resetState();
    } else {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                divergence[i][j] = divergenceAt(i, j);
            }
        }
    }

    // Solve and apply pressure
    if (enablePressureSolve) {
        solvePressureField();
        applyPressure();
    }

    // Advect density and temperature through velocity
    if (gpuImplementation) {
        //loadScalarFieldIntoTexture(densitySlab.ping.textureHandle, density);

        advect(velocitySlab.ping, densitySlab.ping, densitySlab.pong, DENSITY_DISSIPATION);
        //copyScalarTextureIntoField(densitySlab.pong.textureHandle, density);
        swapSurfaces(densitySlab);
        resetState();

        //loadScalarFieldIntoTexture(temperatureSlab.ping.textureHandle, temperature);

        advect(velocitySlab.ping, temperatureSlab.ping, temperatureSlab.pong, TEMPERATURE_DISSIPATION);
        //copyScalarTextureIntoField(temperatureSlab.pong.textureHandle, temperature);
        swapSurfaces(temperatureSlab);
        resetState();
    } else {
        // Compute the trace position
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                tracePosition[i][j] = traceParticle(i * gridSpacing, j * gridSpacing);
            }
        }

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

    float forceFactor = PULSE_FORCE;
    float addAmount = 0.5f;
    if (gpuImplementation) {
        forceFactor /= 5.0f;
        addAmount /= 2.0f;
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * horizontalSpacing;
            float y = j * verticalSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < PULSE_RANGE) {
                float falloffFactor = (1 - glm::distance(position, gridPosition) / PULSE_RANGE);
                velocity[i][j] = forceFactor * (randomPulseAngle ? glm::normalize(force) : glm::normalize(gridPosition - position));
                density[i][j] += addAmount * falloffFactor;
                temperature[i][j] += addAmount * 5 * falloffFactor;
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
    if (gpuImplementation) {
        clearSurface(pressureSlab.ping, 0.0f);
        clearSurface(pressureSlab.pong, 0.0f);

        // Iteratively solve the new pressure field
        for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
            jacobi(divergenceSlab.ping, pressureSlab.ping, pressureSlab.pong);
            swapSurfaces(pressureSlab);
        }

        //copyScalarTextureIntoField(pressureSlab.ping.textureHandle, pressure);
        resetState();
    } else {
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
}

float SmokeSimulation::pressureAt(int i, int j) {
    float d = divergence[i][j];
    float p = getGridPressure(i + 2, j) +
              getGridPressure(i - 2, j) +
              getGridPressure(i, j + 2) +
              getGridPressure(i, j - 2);
    return (d + p) * 0.25f;
}

void SmokeSimulation::applyPressure() {
    if (gpuImplementation) {
        applyPressure(pressureSlab.ping, velocitySlab.ping, velocitySlab.pong);
        //copyVectorTextureIntoField(velocitySlab.pong.textureHandle, velocity);
        swapSurfaces(velocitySlab);
        resetState();
    } else {
        float a = -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing));

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                float xChange = getGridPressure(i + 1, j) - getGridPressure(i - 1, j);
                float yChange = getGridPressure(i, j + 1) - getGridPressure(i, j - 1);

                velocity[i][j].x += a * xChange;
                velocity[i][j].y += a * yChange;
            }
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
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glUseProgram(smokeShaders[currentShader]);

    // Pass uniforms
    GLint screenWidthLocation = glGetUniformLocation(smokeShaders[currentShader], "screenWidth");
    GLint screenHeightLocation = glGetUniformLocation(smokeShaders[currentShader], "screenHeight");
    glUniform1i(screenWidthLocation, SCREEN_WIDTH);
    glUniform1i(screenHeightLocation, SCREEN_HEIGHT);

    GLint textureALocation = glGetUniformLocation(smokeShaders[currentShader], "textureA");
    GLint textureBLocation = glGetUniformLocation(smokeShaders[currentShader], "textureB");
    glUniform1i(textureALocation, 0);
    glUniform1i(textureBLocation, 1);

    if (gpuImplementation) {
        switch (currentShader) {
            case 0:
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, densitySlab.ping.textureHandle);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, temperatureSlab.ping.textureHandle);
                break;
            case 1:
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, densitySlab.ping.textureHandle);
                break;
            case 2:
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, velocitySlab.ping.textureHandle);
                break;
            case 3:
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, temperatureSlab.ping.textureHandle);
                break;
            case 4:
                // glActiveTexture(GL_TEXTURE0);
                // glBindTexture(GL_TEXTURE_2D, curlSlab.ping.textureHandle);
                break;
            default:
                break;
        }
    } else {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                textureFieldA[i][j][0] = 0.0f;
                textureFieldA[i][j][1] = 0.0f;
                textureFieldB[i][j][0] = 0.0f;
                textureFieldB[i][j][1] = 0.0f;

                switch (currentShader) {
                    case 0:
                        textureFieldA[i][j][0] = density[j][i];
                        textureFieldB[i][j][0] = temperature[j][i];
                        break;
                    case 1:
                        textureFieldA[i][j][0] = density[j][i];
                        break;
                    case 2:
                        textureFieldA[i][j][0] = velocity[j][i].x / 10.0f;
                        textureFieldA[i][j][1] = velocity[j][i].y / 10.0f;
                        break;
                    case 3:
                        textureFieldA[i][j][0] = temperature[j][i];
                        break;
                    case 4:
                        textureFieldA[i][j][0] = curl[j][i];
                        break;
                    default:
                        break;
                }
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureA);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &textureFieldA[0][0][0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &textureFieldB[0][0][0]);
    }

    drawFullscreenQuad();
}

void SmokeSimulation::renderVelocityField(glm::mat4 transform, glm::vec2 mousePosition) {
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

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
