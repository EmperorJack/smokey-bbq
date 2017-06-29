#ifndef SMOKE_SIMULATION_HPP
#define SMOKE_SIMULATION_HPP

class SmokeSimulation {

    struct Surface {
        GLuint fboHandle;
        GLuint textureHandle;
        int numComponents;
    };

    struct Slab {
        Surface ping;
        Surface pong;
    };

    // Programs
    GLuint advectProgram;
    GLuint applyImpulseProgram;
    GLuint applyBuoyancyProgram;
    GLuint computeDivergenceProgram;
    GLuint jacobiProgram;
    GLuint applyPressureProgram;

    // Slabs
    Slab velocitySlab;
    Slab divergenceSlab;
    Slab pressureSlab;
    Slab densitySlab;
    Slab temperatureSlab;

    void init();
    void initPrograms();
    void initSlabs();

    Slab createSlab(int width, int height, int numComponents);
    Surface createSurface(int width, int height, int numComponents);
    void swapSurfaces(Slab *slab);
    void clearSurface(Surface s, float v);
    void resetState();
    void drawFullscreenQuad();

    void advect(Surface velocity, Surface source, Surface destination, float dissipation);

    void loadVelocityIntoTexture();

    public:

        // Constants
        static constexpr int GRID_SIZE = 16;
        static constexpr float TIME_STEP = 0.1f;
        static constexpr float FLUID_DENSITY = 1.0f;
        static constexpr float STROKE_WEIGHT = 2.0f;
        static constexpr float PULSE_RANGE = 50.0f;
        static constexpr float EMITTER_RANGE = 80.0f;
        static constexpr float PULSE_FORCE = 150.0f;
        static constexpr float VELOCITY_DISSIPATION = 0.98;
        static constexpr float DENSITY_DISSIPATION = 0.97; // 0.987
        static constexpr float TEMPERATURE_DISSIPATION = 0.96f;
        static constexpr int JACOBI_ITERATIONS = 100;
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
        void emit(glm::vec2 position, glm::vec2 force, float range, float density, float temperature);

        // Toggle variables
        bool enableEmitter = false;
        bool enablePressureSolve = false;
        bool randomPulseAngle = false;
        bool enableBuoyancy = false;
        bool wrapBorders = false;

    private:

        // Grid fields
        glm::vec2 velocity[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        glm::vec2 advectedVelocity[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float divergence[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float pressure[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float newPressure[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float density[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float advectedDensity[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float temperature[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        float advectedTemperatue[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
        glm::vec2 tracePosition[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];

        // Instance variables
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
