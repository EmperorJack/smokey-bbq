#ifndef SMOKE_SIMULATION_HPP
#define SMOKE_SIMULATION_HPP

#include <opengl.hpp>

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
    static constexpr int GRID_SIZE = 32;

    // Variables
    float TIME_STEP;
    float FLUID_DENSITY;
    int JACOBI_ITERATIONS;
    float GRAVITY;
    float PULSE_RANGE;
    float EMITTER_RANGE;
    float PULSE_FORCE;
    float VELOCITY_DISSIPATION;
    float DENSITY_DISSIPATION;
    float TEMPERATURE_DISSIPATION;
    float RISE_FORCE;
    float FALL_FORCE;
    float ATMOSPHERE_TEMPERATURE;
    float STROKE_WEIGHT;
    float VORTICITY_CONFINEMENT_FORCE;

    // Setup
    SmokeSimulation();
    void setDefaultVariables();
    void setDefaultToggles();
    void resetFields();

    // Display toggle
    int currentShader;

    // Updating
    void update();

    // Rendering
    void render(glm::mat4 transform, glm::vec2 mousePosition);

    // Interactions
    void addPulse(glm::vec2);
    void emit(glm::vec2 position, glm::vec2 force, float range, float density, float temperature);

    // Toggle variables
    bool displayDensityField;
    bool displayVelocityField;
    bool updateSimulation;
    bool enableEmitter;
    bool enablePressureSolve;
    bool randomPulseAngle;
    bool enableBuoyancy;
    bool wrapBorders;
    bool enableVorticityConfinement;

    bool gpuImplementation = false;

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
    float curl[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];

    // Instance variables
    float gridSpacing;

    // VBOs
    GLuint squareVBO;
    GLuint velocityVBO;
    GLuint densityVBO;

    // Textures
    GLuint texture;

    // Shaders
    GLuint simpleShader;
    std::vector<GLuint> smokeShaders;

    // Fluid dynamics
    glm::vec2 getVelocity(float x, float y);
    float getDensity(float x, float y);
    float getTemperature(float x, float y);

    glm::vec2 traceParticle(float x, float y);

    float divergenceAt(int i, int j);
    void solvePressureField();
    float pressureAt(int i, int j);
    void applyPressure();

    glm::vec2 buoyancyForceAt(int i, int j);

    float curlAt(int i, int j);
    glm::vec2 vorticityConfinementForceAt(int i, int j);

    float getInterpolatedVelocity(float x, float y, bool xAxis);
    float getInterpolatedDensity(float x, float y);
    float getInterpolatedTemperature(float x, float y);

    glm::vec2 getGridVelocity(int i, int j);
    float getGridDensity(int i, int j);
    float getGridTemperature(int i, int j);
    float getGridPressure(int i, int j);
    float getGridCurl(int i, int j);

    // Indexing
    int wrapIndex(int i);
    int clampIndex(int i);
    bool clampBoundary(int &i);

    // Rendering
    void renderField();
    void renderVelocityField(glm::mat4 transform, glm::vec2 mousePosition);
    void drawLine(glm::mat4);

};

#endif
