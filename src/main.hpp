#ifndef MAIN_HPP
#define MAIN_HPP

#include <random>

// Screen settings
#define FULL_SCREEN false

#if FULL_SCREEN
static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 800;
#else
static const int SCREEN_WIDTH = 1000;
static const int SCREEN_HEIGHT = 1000;
#endif

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
