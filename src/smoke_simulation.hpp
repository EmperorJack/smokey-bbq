#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        static constexpr int GRID_SIZE = 50;
        static constexpr float TIME_STEP = 1.0f;
        static constexpr bool WRAP_BORDERS = true;
        static constexpr float STROKE_WEIGHT = 2.0f;
        static constexpr float PULSE_RANGE = 200.0f;

        SmokeSimulation(float);

        void setupFields();
        void update();

        glm::vec2 getVelocity(float x, float y);
        float getDensity(float x, float y);

        glm::vec2 advectedVelocityAt(int i, int j);
        float advectedDensityAt(int i, int j);

        glm::vec2 traceParticle(float x, float y, float timeStep);

        float getInterpolatedValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], float x, float y, int);
        glm::vec2 getGridValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], int i, int j);

        void addPulse(glm::vec2);
        void toggleVectorDisplay();
        void toggleDensityDisplay();

        void render(glm::mat4, glm::vec2);
        void drawSquare(glm::mat4, bool);
        void drawLine(glm::mat4);

        float myRandom();
};

#endif
