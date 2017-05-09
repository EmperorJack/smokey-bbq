#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        static constexpr int GRID_SIZE = 64;
        static constexpr float TIME_STEP = 5.0f;
        static constexpr float FLUID_DENSITY = 1.0f;
        static constexpr bool WRAP_BORDERS = false;
        static constexpr float STROKE_WEIGHT = 2.0f;
        static constexpr float PULSE_RANGE = 200.0f;
        static constexpr float PULSE_FORCE = 5.0f;
        static constexpr float DENSITY_DISSAPATION = 0.94;
        static constexpr int JACOBI_ITERATIONS = 10;

        SmokeSimulation(float);

        void setupFields();
        void update();

        glm::vec2 getVelocity(float x, float y);
        float getDensity(float x, float y);

        glm::vec2 advectedVelocityAt(int i, int j);
        float advectedDensityAt(int i, int j);

        glm::vec2 traceParticle(float x, float y);

        float divergenceAt(int i, int j);
        void solvePressureField();
        float pressureAt(int i, int j);
        void applyPressure();

        float getInterpolatedValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], float x, float y, int);
        glm::vec2 getGridValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], int i, int j);

        void addPulse(glm::vec2);
        void toggleVectorDisplay();
        void toggleDensityDisplay();
        void toggleEnableEmitter();
        void togglePressureSolve();

        void render(glm::mat4, glm::vec2);
        void drawDensity(glm::mat4);
        void drawSquare(glm::mat4, bool);
        void drawLine(glm::mat4);
};

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

static inline int intFloor(float x) {
    int i = (int) x; /* truncate */
    return i - (i > x); /* convert trunc to floor */
}

#endif
