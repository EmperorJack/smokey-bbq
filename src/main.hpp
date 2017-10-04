#ifndef MAIN_HPP
#define MAIN_HPP

#include <random>
#include <glm/glm.hpp>

// Screen settings
#define FULL_SCREEN false
#define BORDERLESS false
#define VSYNC true

#if FULL_SCREEN || BORDERLESS
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#else
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#endif

#if SCREEN_WIDTH > SCREEN_HEIGHT
static const glm::vec2 windowToGrid = glm::vec2(SCREEN_HEIGHT / (float) SCREEN_WIDTH, 1.0f);
#elif SCREEN_WIDTH < SCREEN_HEIGHT
static const glm::vec2 windowToGrid = glm::vec2(1.0f, SCREEN_WIDTH / (float) SCREEN_HEIGHT);
#else
static const glm::vec2 windowToGrid = glm::vec2(1.0f, 1.0f);
#endif

// Threading
static const int NUM_THREADS = 8;

// Paths
static const char* SHADER_PATH = "resources/shaders/";

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
