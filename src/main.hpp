#ifndef MAIN_HPP
#define MAIN_HPP

// OS settings
static const bool WIN = true;
static const bool MAC = false;
static const bool LINUX = false;

// Screen settings
static const int SCREEN_WIDTH = 1920;
static const int SCREEN_HEIGHT = 1080;
static const bool FULL_SCREEN = true;

// Paths
static const char* SHADER_PATH = "resources/shaders/";

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
