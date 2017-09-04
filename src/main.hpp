#ifndef MAIN_HPP
#define MAIN_HPP

#include <random>
#include <glm/glm.hpp>

// Screen settings
#define FULL_SCREEN true
#define BORDERLESS false

#if FULL_SCREEN
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#else
#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000
#endif

#if SCREEN_WIDTH > SCREEN_HEIGHT
static const glm::vec2 windowToGrid = glm::vec2(SCREEN_HEIGHT / (float) SCREEN_WIDTH, 1.0f);
#elif SCREEN_WIDTH < SCREEN_HEIGHT
static const glm::vec2 windowToGrid = glm::vec2(1.0f, SCREEN_WIDTH / SCREEN_HEIGHT);
#else
static const glm::vec2 windowToGrid = glm::vec2(1.0f, 1.0f);
#endif

// Paths
static const char* SHADER_PATH = "resources/shaders/";

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
