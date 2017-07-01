#ifndef MAIN_HPP
#define MAIN_HPP

// Screen settings
#define FULL_SCREEN false

#if FULL_SCREEN
static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 800;
#else
static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 800;
#endif

// Paths
static const char* SHADER_PATH = "resources/shaders/";

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
