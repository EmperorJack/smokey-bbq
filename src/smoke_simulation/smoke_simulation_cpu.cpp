#include <iostream>
#include <algorithm>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation/smoke_simulation.hpp>

void SmokeSimulation::initCPU() {
    resetFields();

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
}

void SmokeSimulation::resetFields() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            velocity[i][j] = glm::vec2(0.0f, 0.0f);
            advectedVelocity[i][j] = glm::vec2(0.0f, 0.0f);
            divergence[i][j] = 0.0f;
            pressure[i][j] = 0.0f;
            newPressure[i][j] = 0.0f;
            density[i][j] = 0.0f;
            advectedDensity[i][j] = 0.0f;
            temperature[i][j] = atmosphereTemperature;
            advectedTemperatue[i][j] = atmosphereTemperature;
            tracePosition[i][j] = glm::vec2(0.0f, 0.0f);
            curl[i][j] = 0.0f;
            rgb[i][j] = glm::vec3(0.0f, 0.0f, 0.0f);
            advectedRgb[i][j] = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }
}

void SmokeSimulation::updateCPU() {

    // Advect velocity through velocity
    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedVelocity[i][j] = getVelocity(tracePosition[i][j].x, tracePosition[i][j].y) * velocityDissipation;
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            velocity[i][j] = advectedVelocity[i][j];
        }
    }

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 position = glm::vec2(GRID_SIZE / 2 * SCREEN_WIDTH / GRID_SIZE, GRID_SIZE * SCREEN_HEIGHT / GRID_SIZE - 2);
        glm::vec2 force = glm::vec2(myRandom() * pulseForce - pulseForce / 2.0f, -pulseForce);

        emitCPU(position, emitterRange,
                std::vector<Display>{ VELOCITY, DENSITY, TEMPERATURE },
                std::vector<glm::vec3>{ glm::vec3(force, 0.0f), glm::vec3(0.2f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) }
        );
    }

    // Buoyancy
    if (enableBuoyancy) {
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                velocity[i][j] += buoyancyForceAt(i, j);
            }
        }
    }

    // Compute curl
    if (enableVorticityConfinement || computeIntermediateFields) {
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                curl[i][j] = curlAt(i, j);
            }
        }
    }

    // Apply vorticity confinement
    if (enableVorticityConfinement) {
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                velocity[i][j] += vorticityConfinementForceAt(i, j);
            }
        }
    }

    // Compute divergence
    if (enablePressureSolver || computeIntermediateFields) {
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                divergence[i][j] = divergenceAt(i, j);
            }
        }
    }

    // Pressure solver
    if (enablePressureSolver) {

        // Reset the pressure field
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                pressure[i][j] = 0.0f;
            }
        }

        // Iteratively solve the new pressure field
        for (int iteration = 0; iteration < jacobiIterations; iteration++) {
            #pragma omp parallel for
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    newPressure[i][j] = pressureAt(i, j);
                }
            }

            #pragma omp parallel for
            for (int i = 0; i < GRID_SIZE; i++) {
                for (int j = 0; j < GRID_SIZE; j++) {
                    pressure[i][j] = newPressure[i][j];
                }
            }
        }

        float a = -(timeStep / (2 * fluidDensity * gridSpacing));

        // Apply pressure
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                float xChange = getGridPressure(clampIndex(i + 1), j) - getGridPressure(clampIndex(i - 1), j);
                float yChange = getGridPressure(i, clampIndex(j + 1)) - getGridPressure(i, clampIndex(j - 1));

                velocity[i][j].x += a * xChange;
                velocity[i][j].y += a * yChange;
            }
        }
    }

    // Compute the trace position
    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            tracePosition[i][j] = traceParticle(i * gridSpacing, j * gridSpacing);
        }
    }

    // Advect density and temperature through velocity
    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedDensity[i][j] = getDensity(tracePosition[i][j].x, tracePosition[i][j].y) * densityDissipation;
            advectedTemperatue[i][j] = getTemperature(tracePosition[i][j].x, tracePosition[i][j].y) * temperatureDissipation;
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            density[i][j] = advectedDensity[i][j];
            temperature[i][j] = advectedTemperatue[i][j];
        }
    }

    // Advect rgb through velocity if enabled
    if (std::find(compositionFields.begin(), compositionFields.end(), RGB) != compositionFields.end()) {
        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                advectedRgb[i][j] = getRgb(tracePosition[i][j].x, tracePosition[i][j].y) * rgbDissipation;
            }
        }

        #pragma omp parallel for
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                rgb[i][j] = advectedRgb[i][j];
            }
        }
    }
}

void SmokeSimulation::emitCPU(glm::vec2 position, float range, std::vector<Display> fields, std::vector<glm::vec3> values) {
    position *= windowToGrid;

    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            glm::vec2 gridPosition = glm::vec2(i * gridSpacing, j * gridSpacing);
            float distance = glm::distance(position, gridPosition);

            if (distance < range) {
                float falloff = (1.0f - distance / range);

                for (int field = 0; field < fields.size(); field++) {
                    switch(fields[field]) {
                        case DENSITY:
                            density[i][j] += values[field].x * falloff;
                            break;
                        case VELOCITY:
                            velocity[i][j] += glm::vec2(values[field]) * falloff;
                            break;
                        case TEMPERATURE:
                            temperature[i][j] += values[field].x * falloff;
                            break;
                        case RGB:
                            rgb[i][j] += values[field] * falloff;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
}

glm::vec2 SmokeSimulation::traceParticle(float x, float y) {
    glm::vec2 v = getVelocity(x, y);
    v = getVelocity(x + 0.5f * timeStep * v.x, y + 0.5f * timeStep * v.y);
    return glm::vec2(x, y) - (timeStep * v);
}

float SmokeSimulation::divergenceAt(int i, int j) {
    float a = -((2 * gridSpacing * fluidDensity) / timeStep);

    float b = getVelocity((i + 1) * gridSpacing, j * gridSpacing).x -
              getVelocity((i - 1) * gridSpacing, j * gridSpacing).x +
              getVelocity(i * gridSpacing, (j + 1) * gridSpacing).y -
              getVelocity(i * gridSpacing, (j - 1) * gridSpacing).y;

    return a * b;
}

glm::vec2 SmokeSimulation::buoyancyForceAt(int i, int j) {
    return (fallForce * density[i][j] - riseForce * (temperature[i][j] - atmosphereTemperature)) * glm::vec2(0.0f, gravity / abs(gravity));
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
    glm::vec3 force = timeStep * vorticityConfinementForce * glm::cross(magnitude, curlVector);

    return glm::vec2(force);
}

float SmokeSimulation::pressureAt(int i, int j) {
    float d = divergence[i][j];
    float p = getGridPressure(clampIndex(i + 2), j) +
              getGridPressure(clampIndex(i - 2), j) +
              getGridPressure(i, clampIndex(j + 2)) +
              getGridPressure(i, clampIndex(j - 2));
    return (d + p) * 0.25f;
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

glm::vec3 SmokeSimulation::getRgb(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedRgb(normX, normY);
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

glm::vec3 SmokeSimulation::getInterpolatedRgb(float x, float y) {
    int i = ((int) (x + GRID_SIZE)) - GRID_SIZE;
    int j = ((int) (y + GRID_SIZE)) - GRID_SIZE;

    return (i+1-x) * (j+1-y) * getGridRgb(i, j) +
           (x-i) * (j+1-y)   * getGridRgb(i+1, j) +
           (i+1-x) * (y-j)   * getGridRgb(i, j+1) +
           (x-i) * (y-j)     * getGridRgb(i+1, j+1);
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

glm::vec3 SmokeSimulation::getGridRgb(int i, int j) {
    if (wrapBorders) {
        return rgb[wrapIndex(i)][wrapIndex(j)];
    } else {
        bool boundary = clampBoundary(i) || clampBoundary(j);
        return rgb[i][j] * (boundary ? 0.0f : 1.0f);
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

int SmokeSimulation::clampIndex(int i) {
    if (i < 0 && !wrapBorders) {
        return 0;
    }  else if (i >= GRID_SIZE && !wrapBorders) {
        return GRID_SIZE - 1;
    }

    return i;
}

void SmokeSimulation::renderCPU() {
    #pragma omp parallel for
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            textureFieldA[i][j] = glm::vec3();
            textureFieldB[i][j] = glm::vec3();

            switch (currentDisplay) {
                case COMPOSITION:
                    textureFieldA[i][j] = dataForDisplayCPU(compositionFields[0], j, i);
                    textureFieldB[i][j] = dataForDisplayCPU(compositionFields[1], j, i);
                    break;
                case DENSITY:
                    textureFieldA[i][j].r = density[j][i];
                    break;
                case VELOCITY:
                    textureFieldA[i][j].r = velocity[j][i].x;
                    textureFieldA[i][j].g = velocity[j][i].y;
                    break;
                case TEMPERATURE:
                    textureFieldA[i][j].r = temperature[j][i];
                    break;
                case CURL:
                    textureFieldA[i][j].r = curl[j][i];
                    break;
                default:
                    break;
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, 0, GL_RGB, GL_FLOAT, &textureFieldA[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, 0, GL_RGB, GL_FLOAT, &textureFieldB[0]);
}

glm::vec3 SmokeSimulation::dataForDisplayCPU(Display display, int i, int j) {
    switch (display) {
        case DENSITY:
            return glm::vec3(density[i][j], 0, 0);
        case TEMPERATURE:
            return glm::vec3(temperature[i][j], 0, 0);
        case CURL:
            return glm::vec3(curl[i][j], 0, 0);
        case RGB:
            return rgb[i][j];
        default:
            break;
    }

    return glm::vec3();
}
