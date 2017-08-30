#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

void SmokeSimulation::initGPU() {
    initPrograms();
    initSlabs();

    // Setup samplers for bounded vs border wrapping
    GLuint boundedSampler;
    glGenSamplers(1, &boundedSampler);
    glSamplerParameteri(boundedSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(boundedSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(boundedSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(boundedSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint wrapBordersSampler;
    glGenSamplers(1, &wrapBordersSampler);
    glSamplerParameteri(wrapBordersSampler, GL_TEXTURE_WRAP_S, GL_WRAP_BORDER);
    glSamplerParameteri(wrapBordersSampler, GL_TEXTURE_WRAP_T, GL_WRAP_BORDER);
    glSamplerParameteri(wrapBordersSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(wrapBordersSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    updateSampler();
}

void SmokeSimulation::initPrograms() {
    advectProgram = loadShaders("SmokeVertexShader", "advect");
    applyImpulseProgram = loadShaders("SmokeVertexShader", "applyImpulse");
    applyBuoyancyProgram = loadShaders("SmokeVertexShader", "applyBuoyancy");
    computeCurlProgram = loadShaders("SmokeVertexShader", "computeCurl");
    applyVorticityConfinementProgram = loadShaders("SmokeVertexShader", "applyVorticityConfinement");
    computeDivergenceProgram = loadShaders("SmokeVertexShader", "computeDivergence");
    jacobiProgram = loadShaders("SmokeVertexShader", "jacobi");
    applyPressureProgram = loadShaders("SmokeVertexShader", "applyPressure");
}

void SmokeSimulation::initSlabs() {
    velocitySlab = createSlab(GRID_SIZE, GRID_SIZE, 2);
    densitySlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    temperatureSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    curlSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    divergenceSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    pressureSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);

    slabs.push_back(velocitySlab);
    slabs.push_back(densitySlab);
    slabs.push_back(temperatureSlab);
    slabs.push_back(curlSlab);
    slabs.push_back(divergenceSlab);
    slabs.push_back(pressureSlab);

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (numComponents) {
        case 1: glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_HALF_FLOAT, 0); break;
        case 2: glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_HALF_FLOAT, 0); break;
        case 3: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, 0); break;
        case 4: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, 0); break;
        default: fprintf(stderr, "Invalid slab format."); exit(1);
    }

    GLuint colorBuffer;
    glGenRenderbuffers(1, &colorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
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

void SmokeSimulation::updateSampler() {
    glBindSampler(0, wrapBorders ? wrapBordersSampler : boundedSampler);
    glBindSampler(1, wrapBorders ? wrapBordersSampler : boundedSampler);
    glBindSampler(2, wrapBorders ? wrapBordersSampler : boundedSampler);
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
    for (Slab slab : slabs) {
        clearSurface(slab.ping, 0.0f);
        clearSurface(slab.pong, 0.0f);
    }
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

    // Update texture parameters if needed
    if (prevWrapBorders != wrapBorders) {
        updateSampler();
    }
    prevWrapBorders = wrapBorders;

    // Advect velocity through velocity
    advect(velocitySlab.ping, velocitySlab.ping, velocitySlab.pong, VELOCITY_DISSIPATION);
    swapSurfaces(velocitySlab);
    resetState();

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 position = glm::vec2(GRID_SIZE / 2 * SCREEN_WIDTH / GRID_SIZE, GRID_SIZE * SCREEN_HEIGHT / GRID_SIZE - 2);
        glm::vec2 force = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);

        emitGPU(position, force, EMITTER_RANGE, 0.2f, 1.0f);
    }

    // Buoyancy
    if (enableBuoyancy) {
        applyBuoyancy(temperatureSlab.ping, densitySlab.ping, velocitySlab.ping);
        resetState();
    }

    // Compute curl
    if (enableVorticityConfinement || computeIntermediateFields) {
        computeCurl(velocitySlab.ping, curlSlab.pong);
        swapSurfaces(curlSlab);
        resetState();
    }

    // Apply vorticity confinement
    if (enableVorticityConfinement) {
        applyVorticityConfinement(curlSlab.ping, velocitySlab.ping);
        resetState();
    }

    // Compute divergence
    if (enablePressureSolver || computeIntermediateFields) {
        computeDivergence(velocitySlab.ping, divergenceSlab.pong);
        swapSurfaces(divergenceSlab);
        resetState();
    }

    // Pressure solver
    if (enablePressureSolver) {

        // Reset the pressure field
        clearSurface(pressureSlab.ping, 0.0f);
        clearSurface(pressureSlab.pong, 0.0f);

        // Iteratively solve the new pressure field
        for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
            jacobi(divergenceSlab.ping, pressureSlab.ping, pressureSlab.pong);
            swapSurfaces(pressureSlab);
        }

        resetState();

        // Apply pressure
        applyPressure(pressureSlab.ping, velocitySlab.ping);
        resetState();
    }

    // Advect density and temperature through velocity
    advect(velocitySlab.ping, densitySlab.ping, densitySlab.pong, DENSITY_DISSIPATION);
    swapSurfaces(densitySlab);
    resetState();

    advect(velocitySlab.ping, temperatureSlab.ping, temperatureSlab.pong, TEMPERATURE_DISSIPATION);
    swapSurfaces(temperatureSlab);
    resetState();
}

void SmokeSimulation::emitGPU(glm::vec2 position, glm::vec2 force, float range, float densityAmount, float temperatureAmount) {
    applyImpulse(velocitySlab.ping, position, range, glm::vec3(force, 0.0f), false);
    applyImpulse(densitySlab.ping, position, range, glm::vec3(densityAmount, 0.0f, 0.0f), false);
    applyImpulse(temperatureSlab.ping, position, range, glm::vec3(temperatureAmount, 0.0f, 0.0f), false);

    resetState();
}

void SmokeSimulation::advect(Surface velocitySurface, Surface source, Surface destination, float dissipation) {
    GLuint program = advectProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint wrapBordersLocation = glGetUniformLocation(program, "wrapBorders");
    GLint gridSpacingLocation = glGetUniformLocation(program, "gridSpacing");
    GLint timeStepLocation = glGetUniformLocation(program, "timeStep");
    GLint dissipationLocation = glGetUniformLocation(program, "dissipation");
    GLint velocityTextureLocation = glGetUniformLocation(program, "velocityTexture");
    GLint sourceTextureLocation = glGetUniformLocation(program, "sourceTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(wrapBordersLocation, wrapBorders);
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
    GLint wrapBordersLocation = glGetUniformLocation(program, "wrapBorders");
    GLint gridSpacingLocation = glGetUniformLocation(program, "gridSpacing");
    GLint gradientScaleLocation = glGetUniformLocation(program, "gradientScale");
    GLint velocityTextureLocation = glGetUniformLocation(program, "velocityTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(wrapBordersLocation, wrapBorders);
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
    GLint wrapBordersLocation = glGetUniformLocation(program, "wrapBorders");
    GLint divergenceTextureLocation = glGetUniformLocation(program, "divergenceTexture");
    GLint pressureTextureLocation = glGetUniformLocation(program, "pressureTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(wrapBordersLocation, wrapBorders);
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
    GLint wrapBordersLocation = glGetUniformLocation(program, "wrapBorders");
    GLint gradientScaleLocation = glGetUniformLocation(program, "gradientScale");
    GLint pressureTextureLocation = glGetUniformLocation(program, "pressureTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(wrapBordersLocation, wrapBorders);
    glUniform1f(gradientScaleLocation, -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing)));
    glUniform1i(pressureTextureLocation, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, velocityDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pressureSurface.textureHandle);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    drawFullscreenQuad();
    glDisable(GL_BLEND);
}

void SmokeSimulation::applyImpulse(Surface destination, glm::vec2 position, float radius, glm::vec3 fill, bool allowOutwardImpulse) {
    GLuint program = applyImpulseProgram;
    glUseProgram(program);

    GLint horizontalSpacingLocation = glGetUniformLocation(program, "horizontalSpacing");
    GLint verticalSpacingLocation = glGetUniformLocation(program, "verticalSpacing");
    GLint positionLocation = glGetUniformLocation(program, "position");
    GLint radiusLocation = glGetUniformLocation(program, "radius");
    GLint fillLocation = glGetUniformLocation(program, "fill");
    GLint outwardImpulseLocation = glGetUniformLocation(program, "outwardImpulse");

    glUniform1f(horizontalSpacingLocation, ((float) SCREEN_WIDTH) / GRID_SIZE);
    glUniform1f(verticalSpacingLocation, ((float) SCREEN_HEIGHT) / GRID_SIZE);
    glUniform2f(positionLocation, position.x, position.y);
    glUniform1f(radiusLocation, radius);
    glUniform3f(fillLocation, fill.x, fill.y, fill.z);
    glUniform1i(outwardImpulseLocation, allowOutwardImpulse && !randomPulseAngle);

    glBindFramebuffer(GL_FRAMEBUFFER, destination.fboHandle);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
    glBlendFunc(GL_ONE, GL_ONE);
    drawFullscreenQuad();
    glDisable(GL_BLEND);
}

void SmokeSimulation::computeCurl(Surface velocitySurface, Surface curlSurface) {
    GLuint program = computeCurlProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint wrapBordersLocation = glGetUniformLocation(program, "wrapBorders");
    GLint gridSpacingLocation = glGetUniformLocation(program, "gridSpacing");
    GLint velocityTextureLocation = glGetUniformLocation(program, "velocityTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(wrapBordersLocation, wrapBorders);
    glUniform1f(gridSpacingLocation, gridSpacing);
    glUniform1i(velocityTextureLocation, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, curlSurface.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, velocitySurface.textureHandle);

    drawFullscreenQuad();
}

void SmokeSimulation::applyVorticityConfinement(Surface curlSurface, Surface velocityDestination) {
    GLuint program = applyVorticityConfinementProgram;
    glUseProgram(program);

    GLint gridSizeLocation = glGetUniformLocation(program, "gridSize");
    GLint inverseSizeLocation = glGetUniformLocation(program, "inverseSize");
    GLint wrapBordersLocation = glGetUniformLocation(program, "wrapBorders");
    GLint timeStepLocation = glGetUniformLocation(program, "timeStep");
    GLint vorticityConfinementForceLocation = glGetUniformLocation(program, "vorticityConfinementForce");
    GLint curlTextureLocation = glGetUniformLocation(program, "curlTexture");

    glUniform1i(gridSizeLocation, GRID_SIZE);
    glUniform1f(inverseSizeLocation, 1.0f / GRID_SIZE);
    glUniform1f(wrapBordersLocation, wrapBorders);
    glUniform1f(timeStepLocation, TIME_STEP);
    glUniform1f(vorticityConfinementForceLocation, VORTICITY_CONFINEMENT_FORCE);
    glUniform1i(curlTextureLocation, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, velocityDestination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, curlSurface.textureHandle);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
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
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, curlSlab.ping.textureHandle);
            break;
        default:
            break;
    }
}
