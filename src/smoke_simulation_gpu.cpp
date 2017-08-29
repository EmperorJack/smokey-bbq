#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

void SmokeSimulation::initGPU() {
    initPrograms();
    initSlabs();

    glBlendFunc(GL_ONE, GL_ONE);
}

void SmokeSimulation::initPrograms() {
    advectProgram = loadShaders("SmokeVertexShader", "advect");
    applyImpulseProgram = loadShaders("SmokeVertexShader", "applyImpulse");
    applyBuoyancyProgram = loadShaders("SmokeVertexShader", "applyBuoyancy");
    //computeCurlProgram = loadShaders("SmokeVertexShader", "computeCurl");
    //applyVorticityConfinementProgram = loadShaders("SmokeVertexShader", "applyVorticityConfinement");
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

    resetSlabs();
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

void SmokeSimulation::resetSlabs() {
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

void SmokeSimulation::resetState() {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SmokeSimulation::updateGPU() {
    // Advect velocity through velocity
    //loadVectorFieldIntoTexture(velocitySlab.ping.textureHandle, velocity);

    advect(velocitySlab.ping, velocitySlab.ping, velocitySlab.pong, VELOCITY_DISSIPATION);
    //copyVectorTextureIntoField(velocitySlab.pong.textureHandle, velocity);
    swapSurfaces(velocitySlab);
    resetState();

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 target = glm::vec2(GRID_SIZE / 2 * gridSpacing, GRID_SIZE * gridSpacing - 2);
        glm::vec2 force = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);

        swapSurfaces(velocitySlab);
        applyImpulse(velocitySlab.pong, target / gridSpacing, EMITTER_RANGE / gridSpacing, glm::vec3(force, 0.0f));
        //copyVectorTextureIntoField(velocitySlab.pong.textureHandle, velocity);
        swapSurfaces(velocitySlab);
        resetState();

        swapSurfaces(densitySlab);
        applyImpulse(densitySlab.pong, target / gridSpacing, EMITTER_RANGE / gridSpacing, glm::vec3(0.2f, 0.0f, 0.0f));
        //copyScalarTextureIntoField(densitySlab.pong.textureHandle, density);
        swapSurfaces(densitySlab);
        resetState();

        swapSurfaces(temperatureSlab);
        applyImpulse(temperatureSlab.pong, target / gridSpacing, EMITTER_RANGE / gridSpacing, glm::vec3(1.0f, 0.0f, 0.0f));
        //copyScalarTextureIntoField(temperatureSlab.pong.textureHandle, temperature);
        swapSurfaces(temperatureSlab);
        resetState();
    }

    // Buoyancy
    if (enableBuoyancy) {
        swapSurfaces(velocitySlab);
        applyBuoyancy(temperatureSlab.ping, densitySlab.ping, velocitySlab.pong);
        swapSurfaces(velocitySlab);
        resetState();
    }

    // Compute curl

    // Vorticity confinement

    // Compute divergence
    //loadVectorFieldIntoTexture(velocitySlab.ping.textureHandle, velocity);
    computeDivergence(velocitySlab.ping, divergenceSlab.pong);
    //copyScalarTextureIntoField(divergenceSlab.pong.textureHandle, divergence);
    swapSurfaces(divergenceSlab);
    resetState();

    // Solve and apply pressure
    if (enablePressureSolve) {
        clearSurface(pressureSlab.ping, 0.0f);
        clearSurface(pressureSlab.pong, 0.0f);

        // Iteratively solve the new pressure field
        for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
            jacobi(divergenceSlab.ping, pressureSlab.ping, pressureSlab.pong);
            swapSurfaces(pressureSlab);
        }

        //copyScalarTextureIntoField(pressureSlab.ping.textureHandle, pressure);
        resetState();

        swapSurfaces(velocitySlab);
        applyPressure(pressureSlab.ping, velocitySlab.pong);
        //copyVectorTextureIntoField(velocitySlab.pong.textureHandle, velocity);
        swapSurfaces(velocitySlab);
        resetState();
    }

    // Advect density and temperature through velocity
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
}

void SmokeSimulation::advect(Surface velocitySurface, Surface source, Surface destination, float dissipation) {
    GLuint program = advectProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint gridSpacingLocation = glGetUniformLocation(program, "gridSpacing");
    GLint timeStepLocation = glGetUniformLocation(program, "timeStep");
    GLint dissipationLocation = glGetUniformLocation(program, "dissipation");
    GLint velocityTextureLocation = glGetUniformLocation(program, "velocityTexture");
    GLint sourceTextureLocation = glGetUniformLocation(program, "sourceTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(gridSpacingLocation, gridSpacing);
    glUniform1f(timeStepLocation, TIME_STEP);
    glUniform1f(dissipationLocation, dissipation);
    glUniform1i(velocityTextureLocation, 0);
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
    GLint velocityTextureLocation = glGetUniformLocation(program, "velocityTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(gridSpacingLocation, gridSpacing);
    glUniform1f(gradientScaleLocation, -((2 * gridSpacing * FLUID_DENSITY) / TIME_STEP));
    glUniform1i(velocityTextureLocation, 0);

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
    GLint divergenceTextureLocation = glGetUniformLocation(program, "divergenceTexture");
    GLint pressureTextureLocation = glGetUniformLocation(program, "pressureTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1i(divergenceTextureLocation, 0);
    glUniform1i(pressureTextureLocation, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, pressureDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, divergenceSurface.textureHandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pressureSource.textureHandle);

    drawFullscreenQuad();
}

void SmokeSimulation::applyPressure(Surface pressureSurface, Surface velocityDestination) {
    GLuint program = applyPressureProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint gradientScaleLocation = glGetUniformLocation(program, "gradientScale");
    GLint pressureTextureLocation = glGetUniformLocation(program, "pressureTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(gradientScaleLocation, -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing)));
    glUniform1i(pressureTextureLocation, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, velocityDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pressureSurface.textureHandle);

    glEnable(GL_BLEND);
    drawFullscreenQuad();
    glDisable(GL_BLEND);
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

void SmokeSimulation::applyBuoyancy(Surface temperatureSurface, Surface densitySurface, Surface velocityDestination) {
    GLuint program = applyBuoyancyProgram;
    glUseProgram(program);

    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint fallForceLocation = glGetUniformLocation(program, "fallForce");
    GLint riseForceLocation = glGetUniformLocation(program, "riseForce");
    GLint atmosphereTemperatureLocation = glGetUniformLocation(program, "atmosphereTemperature");
    GLint gravityLocation = glGetUniformLocation(program, "gravity");
    GLint temperatureTextureLocation = glGetUniformLocation(program, "temperatureTexture");
    GLint densityTextureLocation = glGetUniformLocation(program, "densityTexture");

    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(fallForceLocation, FALL_FORCE);
    glUniform1f(riseForceLocation, RISE_FORCE);
    glUniform1f(atmosphereTemperatureLocation, ATMOSPHERE_TEMPERATURE);
    glUniform1f(gravityLocation, GRAVITY);
    glUniform1i(temperatureTextureLocation, 0);
    glUniform1i(densityTextureLocation, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, velocityDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, temperatureSurface.textureHandle);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, densitySurface.textureHandle);

    glEnable(GL_BLEND);
    drawFullscreenQuad();
    glDisable(GL_BLEND);
}

void SmokeSimulation::renderGPU() {
    switch (currentSmokeShader) {
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
}
