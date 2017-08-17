#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

void SmokeSimulation::init() {
    initPrograms();
    initSlabs();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SmokeSimulation::initPrograms() {
    advectProgram = loadShaders("SmokeVertexShader", "advection");
    applyImpulseProgram = loadShaders("SmokeVertexShader", "applyImpulse");
    //applyBuoyancyProgram = loadShaders("")
    computeDivergenceProgram = loadShaders("SmokeVertexShader", "computeDivergence");
    jacobiProgram = loadShaders("SmokeVertexShader", "jacobi");
    applyPressureProgram = loadShaders("SmokeVertexShader", "applyPressure");
}

void SmokeSimulation::initSlabs() {
    velocitySlab = createSlab(GRID_SIZE, GRID_SIZE, 2);
    densitySlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    temperatureSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    divergenceSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    pressureSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);

    clearSlabs();
}

void SmokeSimulation::clearSlabs() {
    clearSurface(velocitySlab.ping, 0.0f);
    clearSurface(velocitySlab.pong, 0.0f);
    clearSurface(densitySlab.ping, 0.0f);
    clearSurface(densitySlab.pong, 0.0f);
    clearSurface(temperatureSlab.ping, 0.0f);
    clearSurface(temperatureSlab.pong, 0.0f);
    clearSurface(divergenceSlab.ping, 0.0f);
    clearSurface(divergenceSlab.pong, 0.0f);
    clearSurface(pressureSlab.ping, 0.0f);
    clearSurface(pressureSlab.pong, 0.0f);
}

SmokeSimulation::Slab SmokeSimulation::createSlab(int width, int height, int numComponents) {
    Slab slab;
    slab.pong = createSurface(width, height, numComponents);
    slab.ping = createSurface(width, height, numComponents);
    return slab;
}

SmokeSimulation::Surface SmokeSimulation::createSurface(int width, int height, int numComponents) {
    GLuint fboHandle;
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

    GLuint textureHandle;
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //GL_WRAP_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //GL_WRAP_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (numComponents) {
        case 1: glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0); break;
        case 2: glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, 0); break;
        case 3: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); break;
        case 4: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0); break;
        default: fprintf(stderr, "Invalid slab format."); exit(1);
    }

    GLuint colorbuffer;
    glGenRenderbuffers(1, &colorbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Failed to setup FBO.");
        exit(1);
    }

    Surface surface = { fboHandle, textureHandle, numComponents };

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return surface;
}

void SmokeSimulation::swapSurfaces(Slab &slab) {
    Surface temp = slab.ping;
    slab.ping = slab.pong;
    slab.pong = temp;
}

void SmokeSimulation::clearSurface(Surface s, float v) {
    glBindFramebuffer(GL_FRAMEBUFFER, s.fboHandle);
    glClearColor(v, v, v, v);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SmokeSimulation::advect(Surface velocitySurface, Surface source, Surface destination, float dissipation) {
    GLuint program = advectProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint gridSpacingLocation = glGetUniformLocation(program, "gridSpacing");
    GLint timeStepLocation = glGetUniformLocation(program, "timeStep");
    GLint dissipationLocation = glGetUniformLocation(program, "dissipation");
    GLint sourceTextureLocation = glGetUniformLocation(program, "sourceTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(gridSpacingLocation, gridSpacing);
    glUniform1f(timeStepLocation, TIME_STEP * 5.0f);
    glUniform1f(dissipationLocation, dissipation);
    glUniform1i(sourceTextureLocation, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, destination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, velocitySurface.textureHandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, source.textureHandle);

    drawFullscreenQuad();
}

void SmokeSimulation::computeDivergence(Surface velocitySurface, Surface divergenceSurface) {
    GLuint program = computeDivergenceProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint gridSpacingLocation = glGetUniformLocation(program, "gridSpacing");
    GLint gradientScaleLocation = glGetUniformLocation(program, "gradientScale");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(gridSpacingLocation, gridSpacing);
    glUniform1f(gradientScaleLocation, -((2 * gridSpacing * FLUID_DENSITY) / TIME_STEP));

    glBindFramebuffer(GL_FRAMEBUFFER, divergenceSurface.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, velocitySurface.textureHandle);

    drawFullscreenQuad();
}

void SmokeSimulation::jacobi(Surface divergenceSurface, Surface pressureSource, Surface pressureDestination) {
    GLuint program = jacobiProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint fluidDensityLocation = glGetUniformLocation(program, "fluidDensity");
    GLint pressureTextureLocation = glGetUniformLocation(program, "pressureTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(fluidDensityLocation, FLUID_DENSITY);
    glUniform1i(pressureTextureLocation, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, pressureDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, divergenceSurface.textureHandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pressureSource.textureHandle);

    drawFullscreenQuad();
}

void SmokeSimulation::applyPressure(Surface pressureSurface, Surface velocitySource, Surface velocityDestination) {
    GLuint program = applyPressureProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint gradientScaleLocation = glGetUniformLocation(program, "gradientScale");
    GLint velocityTextureLocation = glGetUniformLocation(program, "velocityTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(gradientScaleLocation, -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing)));
    glUniform1i(velocityTextureLocation, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, velocityDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pressureSurface.textureHandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, velocitySource.textureHandle);

    drawFullscreenQuad();
}


void SmokeSimulation::applyImpulse(Surface destination, glm::vec2 position, float radius, glm::vec3 fill) {
    GLuint program = applyImpulseProgram;
    glUseProgram(program);

    GLint positionLocation = glGetUniformLocation(program, "position");
    GLint radiusLocation = glGetUniformLocation(program, "radius");
    GLint fillLocation = glGetUniformLocation(program, "fill");

    glUniform2f(positionLocation, position.x, position.y);
    glUniform1f(radiusLocation, radius);
    glUniform3f(fillLocation, fill.x, fill.y, fill.z);

    glBindFramebuffer(GL_FRAMEBUFFER, destination.fboHandle);

    glEnable(GL_BLEND);
    drawFullscreenQuad();
    glDisable(GL_BLEND);
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

void SmokeSimulation::resetState() {
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SmokeSimulation::drawFullscreenQuad() {

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
}

void SmokeSimulation::loadFieldIntoVectorTexture(GLuint textureHandle, glm::vec2 field[GRID_SIZE][GRID_SIZE]) {
    float invertField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE][2];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            invertField[i][j][0] = field[j][i].x;
            invertField[i][j][1] = field[j][i].y;
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &invertField[0][0][0]);
}

void SmokeSimulation::loadFieldIntoScalarTexture(GLuint textureHandle, float field[GRID_SIZE][GRID_SIZE]) {
    float invertField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            invertField[i][j] = field[j][i];
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, &invertField[0][0]);
}
