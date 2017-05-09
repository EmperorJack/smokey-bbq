#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        static constexpr int GRID_SIZE = 96;
        static constexpr float TIME_STEP = 0.1f;
        static constexpr float FLUID_DENSITY = 1.0f;
        static constexpr bool WRAP_BORDERS = true;
        static constexpr float STROKE_WEIGHT = 2.0f;
        static constexpr float PULSE_RANGE = 200.0f;
        static constexpr float PULSE_FORCE = 150.0f;
        static constexpr float DENSITY_DISSAPATION = 0.95;
        static constexpr int JACOBI_ITERATIONS = 40;

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

        int iClamp(int i);
        int jClamp(int j);

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

#endif
