#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <main.hpp>
#include <opengl.hpp>
#include <omp.h>
#include <smoke_simulation/smoke_simulation.hpp>
#include <shaderLoader.hpp>

SmokeSimulation::SmokeSimulation() {
    float size = (float) min(SCREEN_WIDTH, SCREEN_HEIGHT);
    gridSpacing = size / GRID_SIZE;

    setDefaultVariables();
    setDefaultToggles();

    // Setup vertex buffer objects
    float lineVertices[] = {
            -strokeWeight / 2.0f, 0.0f,
            -strokeWeight / 2.0f, gridSpacing / 2.5f,
            strokeWeight / 2.0f, gridSpacing / 2.5f,
            strokeWeight / 2.0f, 0.0f,
    };
    glGenBuffers(1, &lineVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

    float fullscreenVertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, 3.0f, 0.0f, 1.0f,
        3.0f, -1.0f, 0.0f, 1.0f
    };
    glGenBuffers(1, &fullscreenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenVertices), fullscreenVertices, GL_STATIC_DRAW);

    // Setup shaders
    simpleShader = loadShaders("SimpleVertexShader", "SimpleFragmentShader");
    currentDisplay = COMPOSITION;
    fieldShaders[DENSITY] = loadShaders("SmokeVertexShader", "fields/DensityFragmentShader");
    fieldShaders[VELOCITY] = loadShaders("SmokeVertexShader", "fields/VelocityFragmentShader");
    fieldShaders[TEMPERATURE] = loadShaders("SmokeVertexShader", "fields/TemperatureFragmentShader");
    fieldShaders[CURL] = loadShaders("SmokeVertexShader", "fields/CurlFragmentShader");

    initCPU();
    initGPU();

    // Sleep for a moment to ensure everything is setup, i.e: GPU
    #ifdef __linux__
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    #endif
}

void SmokeSimulation::setDefaultVariables() {
    timeStep = 0.05f;
    fluidDensity = 1.0f;
    jacobiIterations = 40;

    gravity = 0.0981f;
    pulseRange = 50.0f;
    emitterRange = 80.0f;
    pulseForce = 150.0f;

    velocityDissipation = 0.98f;
    densityDissipation = 0.975f;
    temperatureDissipation = 0.94f;

    riseForce = 1.0f;
    fallForce = 1.0f;
    atmosphereTemperature = 0.0f;

    strokeWeight = 2.0f;

    vorticityConfinementForce = 5.0f;
}

void SmokeSimulation::setDefaultToggles() {
    displaySmokeField = true;
    displayVelocityField = false;
    updateSimulation = true;
    enableEmitter = true;
    enablePressureSolver = true;
    randomPulseAngle = false;
    enableBuoyancy = true;
    wrapBorders = false; prevWrapBorders = wrapBorders;
    enableVorticityConfinement = true;
    computeIntermediateFields = false;
    useCPUMultithreading = true;
    useGPUImplementation = true;
}

void SmokeSimulation::reset() {
    resetFields();
    resetSlabs();
}

void SmokeSimulation::update() {
    if (!updateSimulation) return;

    glViewport(0, 0, GRID_SIZE, GRID_SIZE);

    // Set thread limit
    omp_set_num_threads(useCPUMultithreading ? NUM_THREADS : 1);

    std::chrono::high_resolution_clock::time_point t1;
    std::chrono::high_resolution_clock::time_point t2;

    if (benchmarking) {
        t1 = std::chrono::high_resolution_clock::now();
    }

    if (useGPUImplementation) {
        updateGPU();
    } else {
        updateCPU();
    }

    if (benchmarking) {
        t2 = std::chrono::high_resolution_clock::now();

        double duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        updateTimes.push_back(duration);

        benchmarkSample++;

        if (benchmarkSample >= BENCHMARK_SAMPLES) {
            finishBenchmark();
        }
    }
}

void SmokeSimulation::setCompositionData(GLuint shader, std::vector<Display> fields) {
    compositionShader = shader;
    compositionFields = std::vector<Display>(fields);
}

void SmokeSimulation::beginBenchmark() {
    std::cout << "Beginning benchmark" << std::endl;

    benchmarkSample = 0;
    updateTimes.clear();
    benchmarking = true;
}

void SmokeSimulation::finishBenchmark() {
    benchmarking = false;

    double averageDuration = 0.0;
    for (double duration : updateTimes) {
        averageDuration += duration;
    }

    averageDuration /= (double) BENCHMARK_SAMPLES;
    averageDuration /= 1000.0;

//    std::cout << "Benchmark result: " << averageDuration << " ms" << std::endl;
    std::cout << averageDuration << std::endl;

    benchmarkSample = 0;
    updateTimes.clear();
}

void SmokeSimulation::addPulse(glm::vec2 position) {
    position -= glm::vec2(gridSpacing / 2.0f, gridSpacing / 2.0f);
    position *= windowToGrid;

    float addAmount = 0.5f;

    glm::vec2 force = pulseForce * glm::vec2(1.0f, 0.0f);

    if (randomPulseAngle) {
        force = pulseForce * glm::vec2(myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f);
    }

    if (useGPUImplementation) {
        applyImpulse(velocitySlab.ping, position, pulseRange, glm::vec3(force, 0.0f), true);
        applyImpulse(densitySlab.ping, position, pulseRange, glm::vec3(addAmount, 0.0f, 0.0f), false);
        applyImpulse(temperatureSlab.ping, position, pulseRange, glm::vec3(addAmount * 5, 0.0f, 0.0f), false);
        resetState();
    } else {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                glm::vec2 gridPosition = glm::vec2(i * gridSpacing, j * gridSpacing);
                float distance = glm::distance(position, gridPosition);

                if (distance < pulseRange) {
                    float falloff = 1.0f - distance / pulseRange;
                    velocity[i][j] += randomPulseAngle ? force : pulseForce * glm::normalize(gridPosition - position) * falloff;
                    density[i][j] += addAmount * falloff;
                    temperature[i][j] += addAmount * 5 * falloff;
                }
            }
        }
    }
}

void SmokeSimulation::emit(glm::vec2 position, float range, std::vector<Display> fields, std::vector<glm::vec3> values) {
    if (useGPUImplementation) {
        emitGPU(position, range, fields, values);
    } else {
        emitCPU(position, range, fields, values);
    }
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    if (displaySmokeField) {
        if (RETINA) {
            glViewport(0, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2);
        } else {
            glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        }

        GLuint currentShader = currentDisplay == COMPOSITION ? compositionShader : fieldShaders[currentDisplay];
        glUseProgram(currentShader);

        // Pass screen size uniforms
        GLint screenWidthLocation = glGetUniformLocation(currentShader, "screenWidth");
        GLint screenHeightLocation = glGetUniformLocation(currentShader, "screenHeight");
        glUniform1i(screenWidthLocation, SCREEN_WIDTH);
        glUniform1i(screenHeightLocation, SCREEN_HEIGHT);

        // Pass texture location uniforms
        GLint textureALocation = glGetUniformLocation(currentShader, "textureA");
        GLint textureBLocation = glGetUniformLocation(currentShader, "textureB");
        GLint textureCLocation = glGetUniformLocation(currentShader, "textureC");
        glUniform1i(textureALocation, 0);
        glUniform1i(textureBLocation, 1);
        glUniform1i(textureCLocation, 1);

        if (useGPUImplementation) {
            renderGPU();
        } else {
            renderCPU();
        }

        drawFullscreenQuad();
    }

    if (displayVelocityField && !useGPUImplementation) renderVelocityField(transform, mousePosition);
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

            float magnitude = max(min(glm::length(velocityAmount) / pulseForce * 1.5f, 2.0f), 0.5f);

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
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
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

void SmokeSimulation::drawFullscreenQuad() {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO);
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
}

void SmokeSimulation::copyVectorTextureIntoField(GLuint textureHandle, glm::vec2 field[GRID_SIZE][GRID_SIZE]) {
    GLfloat* pixels = new GLfloat[GRID_SIZE * GRID_SIZE * 2];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, pixels);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float xValue = pixels[(GRID_SIZE * i + j) * 2];
            float yValue = pixels[(GRID_SIZE * i + j) * 2 + 1];

            if (isnan(xValue)) xValue = 0.0f;
            if (isnan(yValue)) yValue = 0.0f;

            field[j][i].x = xValue;
            field[j][i].y = yValue;
        }
    }

    delete[] pixels;
}

void SmokeSimulation::copyScalarTextureIntoField(GLuint textureHandle, float field[GRID_SIZE][GRID_SIZE]) {
    GLfloat* pixels = new GLfloat[GRID_SIZE * GRID_SIZE];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, pixels);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float value = pixels[(GRID_SIZE * i + j)];

            if (isnan(value)) value = 0.0f;

            field[j][i] = value;
        }
    }

    delete[] pixels;
}

void SmokeSimulation::loadVectorFieldIntoTexture(GLuint textureHandle, vec2 **field) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            invertVectorField[i][j][0] = field[j][i].x;
            invertVectorField[i][j][1] = field[j][i].y;
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &invertVectorField[0][0][0]);
}

void SmokeSimulation::loadScalarFieldIntoTexture(GLuint textureHandle, float **field) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            invertScalarField[i][j] = field[j][i];
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, &invertScalarField[0][0]);
}
