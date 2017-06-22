#ifndef SMOKE_SIMULATION_HPP
#define SMOKE_SIMULATION_HPP

class SmokeSimulation {

 public:

        // Constants
        static constexpr int GRID_SIZE = 192;
        static constexpr float TIME_STEP = 0.1f;
        static constexpr float FLUID_DENSITY = 1.0f;
        static constexpr float STROKE_WEIGHT = 2.0f;
        static constexpr float PULSE_RANGE = 50.0f;
        static constexpr float EMITTER_RANGE = 80.0f;
        static constexpr float PULSE_FORCE = 150.0f;
        static constexpr float VELOCITY_DISSAPATION = 0.98;
        static constexpr float DENSITY_DISSAPATION = 0.987;
        static constexpr float TEMPERATURE_DISSAPATION = 0.96f;
        static constexpr int JACOBI_ITERATIONS = 40;
        static constexpr float GRAVITY = 0.0981f;
        static constexpr float RISE_FORCE = 1.0f;
        static constexpr float FALL_FORCE = 1.0f;
        static constexpr float ATMOSPHERE_TEMPERATURE = 0.0f;

        // Setup
        SmokeSimulation();
        void resetFields();

        // Updating
        void update();

        // Rendering
        void renderDensity();
        void renderVelocityField(glm::mat4, glm::vec2);

        // Interactions
        void addPulse(glm::vec2);

        // Toggle variables
        bool enableEmitter = false;
        bool enablePressureSolve = true;
        bool randomPulseAngle = false;
        bool enableBuoyancy = true;
        bool wrapBorders = false;

    private:

        // Structures
        struct gridCell {
            glm::vec2 velocity;
            glm::vec2 advectedVelocity;
            float divergence;
            float pressure;
            float newPressure;
            float density;
            float advectedDensity;
            float temperature;
            float advectedTemperatue;
            glm::vec2 tracePosition;
        };

        // Instance variables
        SmokeSimulation::gridCell grid[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float gridSpacing;

        // VBOs
        GLuint squareVBO;
        GLuint velocityVBO;
        GLuint densityVBO;

        // Textures
        GLuint densityTexture;

        // Shaders
        GLuint simpleShader;
        GLuint densityShader;

        // Fluid dynamics
        glm::vec2 getVelocity(float x, float y);
        float getDensity(float x, float y);
        float getTemperature(float x, float y);

        glm::vec2 traceParticle(float x, float y);

        float divergenceAt(int i, int j);
        void solvePressureField();
        float pressureAt(int i, int j);
        void applyPressure();

        glm::vec2 buoyancyAt(int i, int j);

        float getInterpolatedVelocity(float x, float y, bool xAxis);
        float getInterpolatedDensity(float x, float y);
        float getInterpolatedTemperature(float x, float y);

        glm::vec2 getGridVelocity(int i, int j);
        float getGridDensity(int i, int j);
        float getGridTemperature(int i, int j);
        float getGridPressure(int i, int j);

        // Indexing
        int wrapIndex(int i);
        int clampIndex(int i);
        bool clampBoundary(int &i);

        // Rendering
        void drawLine(glm::mat4);
};

#endif
