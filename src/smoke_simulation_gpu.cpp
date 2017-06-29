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
    advectProgram = loadShaders("Vertex", "advection");
    //applyImpulseProgram = loadShaders("")
    //applyBuoyancyProgram = loadShaders("")
    //computeDivergenceProgram = loadShaders("")
    //jacobiProgram = loadShaders("")
    //applyPressureProgram = loadShaders("")
}

void SmokeSimulation::initSlabs() {
    velocitySlab = createSlab(GRID_SIZE, GRID_SIZE, 2);
    divergenceSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    pressureSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    densitySlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
    temperatureSlab = createSlab(GRID_SIZE, GRID_SIZE, 1);
}

SmokeSimulation::Slab SmokeSimulation::createSlab(int width, int height, int numComponents) {
    Slab slab;
    slab.ping = createSurface(width, height, numComponents);
    slab.pong = createSurface(width, height, numComponents);
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
        fprintf(stderr, "Invalid slab format.");
        exit(1);
    }

    Surface surface = { fboHandle, textureHandle, numComponents };

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return surface;
}

void SmokeSimulation::SwapSurfaces(Slab* slab) {
    Surface temp = slab->ping;
    slab->ping = slab->pong;
    slab->pong = temp;
}

void SmokeSimulation::ClearSurface(Surface s, float v) {
    glBindFramebuffer(GL_FRAMEBUFFER, s.fboHandle);
    glClearColor(v, v, v, v);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SmokeSimulation::advect(Surface velocitySurface, Surface source, Surface destination, float dissipation) {
    GLuint p = advectProgram;
    glUseProgram(p);

    GLint gridSizeLoc = glGetUniformLocation(p, "gridSize");
    GLint gridSpacingLoc = glGetUniformLocation(p, "gridSpacing");
    GLint timeStep = glGetUniformLocation(p, "timeStep");
    GLint dissLoc = glGetUniformLocation(p, "dissipation");
    GLint sourceTexture = glGetUniformLocation(p, "sourceTexture");

    glUniform1f(gridSizeLoc, GRID_SIZE);
    glUniform1f(gridSpacingLoc, gridSpacing);
    glUniform1f(timeStep, TIME_STEP);
    glUniform1f(dissLoc, dissipation);
    glUniform1i(sourceTexture, 1);

    glBindFramebuffer(GL_FRAMEBUFFER, destination.fboHandle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, velocitySurface.textureHandle);
//    loadVelocityIntoTexture();
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &velocity[0][0][0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, source.textureHandle);
//    loadVelocityIntoTexture();
     glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &velocity[0][0][0]);

    int targetIndex = GRID_SIZE / 2;
    std::cout << "(" << velocity[targetIndex][targetIndex].x << ", " << velocity[targetIndex][targetIndex].y << ")";

    drawFullscreenQuad();

//    resetState();
//    return;
    GLfloat* pixels = new GLfloat[GRID_SIZE * GRID_SIZE * 2];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, destination.textureHandle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, pixels);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float xValue = pixels[(GRID_SIZE * i + j) * 2];
            float yValue = pixels[(GRID_SIZE * i + j) * 2 + 1];

            if (isnan(xValue)) xValue = 0.0f;
            if (isnan(yValue)) yValue = 0.0f;

            velocity[i][j].x = xValue;
            velocity[i][j].y = yValue;
        }
    }

    std::cout << " -> (" << velocity[targetIndex][targetIndex].x << ", " << velocity[targetIndex][targetIndex].y << ")" << std::endl;

    resetState();
}

void SmokeSimulation::resetState() {
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
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

void SmokeSimulation::loadVelocityIntoTexture() {
    float field[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE][2];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            field[i][GRID_SIZE - i - j][0] = velocity[j][i].x;
            field[i][GRID_SIZE - i - j][1] = velocity[j][i].y;
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &field[0][0][0]);
}