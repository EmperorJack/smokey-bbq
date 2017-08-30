#include <iostream>
#include <vector>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

SmokeSimulation::SmokeSimulation() {
    float size = (float) min(SCREEN_WIDTH, SCREEN_HEIGHT);
    gridSpacing = size / GRID_SIZE;

    setDefaultVariables();
    setDefaultToggles();

    // Setup vertex buffer objects
    float lineVertices[] = {
            -STROKE_WEIGHT / 2.0f, 0.0f,
            -STROKE_WEIGHT / 2.0f, gridSpacing / 2.5f,
            STROKE_WEIGHT / 2.0f, gridSpacing / 2.5f,
            STROKE_WEIGHT / 2.0f, 0.0f,
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
    currentSmokeShader = 0;
    simpleShader = loadShaders("SimpleVertexShader", "SimpleFragmentShader");

    smokeFieldShaders = std::vector<GLuint>();
    smokeFieldShaders.push_back(loadShaders("SmokeVertexShader", "SmokeFragmentShader"));
    smokeFieldShaders.push_back(loadShaders("SmokeVertexShader", "DensityFragmentShader"));
    smokeFieldShaders.push_back(loadShaders("SmokeVertexShader", "VelocityFragmentShader"));
    smokeFieldShaders.push_back(loadShaders("SmokeVertexShader", "TemperatureFragmentShader"));
    smokeFieldShaders.push_back(loadShaders("SmokeVertexShader", "CurlFragmentShader"));

    initCPU();
    initGPU();
}

void SmokeSimulation::setDefaultVariables() {
    TIME_STEP = 0.05f;
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
    displaySmokeField = true;
    displayVelocityField = false;
    updateSimulation = true;
    enableEmitter = false;
    enablePressureSolver = true;
    randomPulseAngle = false;
    enableBuoyancy = true;
    wrapBorders = false; prevWrapBorders = wrapBorders;
    enableVorticityConfinement = true;
    computeIntermediateFields = false;
    useGPUImplementation = true;
}

void SmokeSimulation::reset() {
    resetFields();
    resetSlabs();
}

void SmokeSimulation::update() {
    if (!updateSimulation) return;

    glViewport(0, 0, GRID_SIZE, GRID_SIZE);

    if (useGPUImplementation) {
        updateGPU();
    } else {
        updateCPU();
    }
}

void SmokeSimulation::addPulse(glm::vec2 position) {
    position -= glm::vec2(gridSpacing / 2.0f, gridSpacing / 2.0f);

    float addAmount = 0.5f;

    glm::vec2 force = PULSE_FORCE * glm::vec2(1.0f, 0.0f);

    if (randomPulseAngle) {
        force = PULSE_FORCE * glm::vec2(myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f);
    }

    float horizontalSpacing = ((float) SCREEN_WIDTH) / GRID_SIZE;
    float verticalSpacing = ((float) SCREEN_HEIGHT) / GRID_SIZE;

    if (useGPUImplementation) {
        applyImpulse(velocitySlab.ping, position, PULSE_RANGE, glm::vec3(force, 0.0f), true);
        applyImpulse(densitySlab.ping, position, PULSE_RANGE, glm::vec3(addAmount, 0.0f, 0.0f), false);
        applyImpulse(temperatureSlab.ping, position, PULSE_RANGE, glm::vec3(addAmount * 5, 0.0f, 0.0f), false);
        resetState();
    } else {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                glm::vec2 gridPosition = glm::vec2(i * horizontalSpacing, j * verticalSpacing);
                float distance = glm::distance(position, gridPosition);

                if (distance < PULSE_RANGE) {
                    float falloff = 1.0f - distance / PULSE_RANGE;
                    velocity[i][j] += randomPulseAngle ? force : PULSE_FORCE * glm::normalize(gridPosition - position) * falloff;
                    density[i][j] += addAmount * falloff;
                    temperature[i][j] += addAmount * 5 * falloff;
                }
            }
        }
    }
}

void SmokeSimulation::emit(glm::vec2 position, glm::vec2 force, float range, float densityAmount, float temperatureAmount) {
    if (useGPUImplementation) {
        emitGPU(position, force, range, densityAmount, temperatureAmount);
    } else {
        emitCPU(position, force, range, densityAmount, temperatureAmount);
    }
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    if (displaySmokeField) {
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        glUseProgram(smokeFieldShaders[currentSmokeShader]);

        // Pass screen size uniforms
        GLint screenWidthLocation = glGetUniformLocation(smokeFieldShaders[currentSmokeShader], "screenWidth");
        GLint screenHeightLocation = glGetUniformLocation(smokeFieldShaders[currentSmokeShader], "screenHeight");
        glUniform1i(screenWidthLocation, SCREEN_WIDTH);
        glUniform1i(screenHeightLocation, SCREEN_HEIGHT);

        // Pass texture location uniforms
        GLint textureALocation = glGetUniformLocation(smokeFieldShaders[currentSmokeShader], "textureA");
        GLint textureBLocation = glGetUniformLocation(smokeFieldShaders[currentSmokeShader], "textureB");
        glUniform1i(textureALocation, 0);
        glUniform1i(textureBLocation, 1);

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
