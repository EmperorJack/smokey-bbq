#ifndef MAIN_HPP
#define MAIN_HPP

// OS settings
static const bool WIN = false;
static const bool MAC = true;
static const bool LINUX = false;

// Screen settings
static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 720;
static const bool FULL_SCREEN = false;

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
