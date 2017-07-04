#ifndef SMOKE_SIMULATION_HPP
#define SMOKE_SIMULATION_HPP

#include <opengl.hpp>

class SmokeSimulation {

public:

    // Constants
    static constexpr int GRID_SIZE = 192;

    // Variables
    float TIME_STEP;
    float FLUID_DENSITY;
    int JACOBI_ITERATIONS;
    float GRAVITY;
    float PULSE_RANGE;
    float EMITTER_RANGE;
    float PULSE_FORCE;
    float VELOCITY_DISSAPATION;
    float DENSITY_DISSAPATION;
    float TEMPERATURE_DISSAPATION;
    float RISE_FORCE;
    float FALL_FORCE;
    float ATMOSPHERE_TEMPERATURE;
    float STROKE_WEIGHT;

    // Setup
    SmokeSimulation();
    void setDefaults();
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
    bool enablePressureSolve = true;
    bool randomPulseAngle = false;
    bool enableBuoyancy = true;
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
