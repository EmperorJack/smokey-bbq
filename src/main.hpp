#ifndef MAIN_HPP
#define MAIN_HPP

// OS settings
static const bool WIN = false;
static const bool MAC = false;
static const bool LINUX = true;

// Screen settings
static const int SCREEN_WIDTH = 1000;
static const int SCREEN_HEIGHT = 500;

static float myRandom() {
    return std::rand() % 100 / 100.0f;
}

#endif
