#ifndef SMOKESIMULATION_H
#define SMOKESIMULATION_H

class SmokeSimulation {

    public:
        static constexpr int GRID_SIZE = 192;
        static constexpr float TIME_STEP = 0.1f;
        static constexpr float FLUID_DENSITY = 1.0f;
        static constexpr float STROKE_WEIGHT = 2.0f;
        static constexpr float PULSE_RANGE = 50.0f;
        static constexpr float EMITTER_RANGE = 80.0f;
        static constexpr float PULSE_FORCE = 150.0f;
        static constexpr float DENSITY_DISSAPATION = 0.987;
        static constexpr int JACOBI_ITERATIONS = 40;

        struct gridCell {
            glm::vec2 velocity;
            glm::vec2 advectedVelocity;
            float divergence;
            float pressure;
            float newPressure;
            float density;
            float advectedDensity;
        };

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

        int clampIndex(int i);

        float getInterpolatedVelocity(float x, float y, bool xAxis);
        glm::vec2 getGridVelocity(int i, int j);

        float getInterpolatedDensity(float x, float y);
        float getGridDensity(int i, int j);

        void addPulse(glm::vec2);
        void toggleVectorDisplay();
        void toggleDensityDisplay();
        void toggleEnableEmitter();
        void togglePressureSolve();
        void togglePulseType();
        void toggleWrapBorders();

        void render(glm::mat4, glm::vec2);
        void drawDensity(glm::mat4);
        void drawSquare(glm::mat4, bool);
        void drawLine(glm::mat4);
};

#endif
