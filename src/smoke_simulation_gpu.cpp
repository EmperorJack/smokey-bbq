#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <shaderLoader.hpp>

void SmokeSimulation::init() {
    initPrograms();
    initSlabs();
}

void SmokeSimulation::initPrograms() {
    advectProgram = loadShaders("SimpleVertexShader", "Advect");
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