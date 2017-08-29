#ifndef SMOKE_SIMULATION_HPP
#define SMOKE_SIMULATION_HPP

#include <opengl.hpp>

class SmokeSimulation {

public:

    // Constants
    static constexpr int GRID_SIZE = 128;

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
    void reset();

    // Display toggle
    int currentSmokeShader;

    // Updating
    void update();

    // Rendering
    void render(glm::mat4 transform, glm::vec2 mousePosition);

    // Interactions
    void addPulse(glm::vec2);
    void emit(glm::vec2 position, glm::vec2 force, float range, float density, float temperature);

    // Toggle variables
    bool displaySmokeField;
    bool displayVelocityField;
    bool updateSimulation;
    bool enableEmitter;
    bool enablePressureSolve;
    bool randomPulseAngle;
    bool enableBuoyancy;
    bool wrapBorders;
    bool enableVorticityConfinement;
    bool useGPUImplementation;

private:

    // Instance variables
    float gridSpacing;

    // Vertex buffer objects
    GLuint lineVBO;
    GLuint fullscreenVBO;

    // Shaders
    GLuint simpleShader;
    std::vector<GLuint> smokeFieldShaders;

    // Rendering
    void drawFullscreenQuad();

    // Implementation transfer fields and functions
    float invertVectorField[GRID_SIZE][GRID_SIZE][2];
    float invertScalarField[GRID_SIZE][GRID_SIZE];
    void copyVectorTextureIntoField(GLuint textureHandle, glm::vec2 field[GRID_SIZE][GRID_SIZE]);
    void copyScalarTextureIntoField(GLuint textureHandle, float field[GRID_SIZE][GRID_SIZE]);
    void loadVectorFieldIntoTexture(GLuint textureHandle, glm::vec2 **field);
    void loadScalarFieldIntoTexture(GLuint textureHandle, float **field);


    // ~~~~~~~~~~~~~~~~~~ //
    // CPU Implementation //
    // ~~~~~~~~~~~~~~~~~~ //


    // Setup
    void initCPU();
    void resetFields();

    // Core
    void updateCPU();
    void renderCPU();

    // Grid fields
    glm::vec2 velocity[GRID_SIZE][GRID_SIZE];
    glm::vec2 advectedVelocity[GRID_SIZE][GRID_SIZE];
    float divergence[GRID_SIZE][GRID_SIZE];
    float pressure[GRID_SIZE][GRID_SIZE];
    float newPressure[GRID_SIZE][GRID_SIZE];
    float density[GRID_SIZE][GRID_SIZE];
    float advectedDensity[GRID_SIZE][GRID_SIZE];
    float temperature[GRID_SIZE][GRID_SIZE];
    float advectedTemperatue[GRID_SIZE][GRID_SIZE];
    glm::vec2 tracePosition[GRID_SIZE][GRID_SIZE];
    float curl[GRID_SIZE][GRID_SIZE];

    // Rendering fields and textures
    float textureFieldA[GRID_SIZE][GRID_SIZE][2];
    float textureFieldB[GRID_SIZE][GRID_SIZE][2];
    GLuint textureA;
    GLuint textureB;

    // Algorithm
    glm::vec2 traceParticle(float x, float y);
    glm::vec2 buoyancyForceAt(int i, int j);
    float curlAt(int i, int j);
    glm::vec2 vorticityConfinementForceAt(int i, int j);
    float divergenceAt(int i, int j);
    float pressureAt(int i, int j);

    // Field access
    glm::vec2 getVelocity(float x, float y);
    float getDensity(float x, float y);
    float getTemperature(float x, float y);

    // Field interpolation
    float getInterpolatedVelocity(float x, float y, bool xAxis);
    float getInterpolatedDensity(float x, float y);
    float getInterpolatedTemperature(float x, float y);

    // Grid access
    glm::vec2 getGridVelocity(int i, int j);
    float getGridDensity(int i, int j);
    float getGridTemperature(int i, int j);
    float getGridPressure(int i, int j);
    float getGridCurl(int i, int j);

    // Indexing
    int wrapIndex(int i);
    bool clampBoundary(int &i);
    int clampIndex(int i);

    // Rendering
    void renderVelocityField(glm::mat4 transform, glm::vec2 mousePosition);
    void drawLine(glm::mat4);


    // ~~~~~~~~~~~~~~~~~~ //
    // GPU Implementation //
    // ~~~~~~~~~~~~~~~~~~ //


    // Structs
    struct Surface {
        GLuint fboHandle;
        GLuint textureHandle;
        int numComponents;
    };
    struct Slab {
        Surface ping;
        Surface pong;
    };

    // Fragment programs
    GLuint advectProgram;
    GLuint applyImpulseProgram;
    GLuint applyBuoyancyProgram;
    GLuint computeCurlProgram;
    GLuint applyVorticityConfinementProgram;
    GLuint computeDivergenceProgram;
    GLuint jacobiProgram;
    GLuint applyPressureProgram;

    // Slabs
    Slab velocitySlab;
    Slab densitySlab;
    Slab temperatureSlab;
    Slab divergenceSlab;
    Slab pressureSlab;

    // Setup
    void initGPU();
    void initPrograms();
    void initSlabs();
    Slab createSlab(int width, int height, int numComponents);
    Surface createSurface(int width, int height, int numComponents);

    // Core
    void updateGPU();
    void renderGPU();

    // State functions
    void swapSurfaces(Slab &slab);
    void clearSurface(Surface s, float v);
    void resetSlabs();
    void resetState();

    // Algorithm
    void advect(Surface velocitySurface, Surface source, Surface destination, float dissipation);
    void applyImpulse(Surface destination, glm::vec2 position, float radius, glm::vec3 fill);
    void applyBuoyancy(Surface temperatureSurface, Surface densitySurface, Surface velocityDestination);
    void computeDivergence(Surface velocitySurface, Surface divergenceSurface);
    void jacobi(Surface divergenceSurface, Surface pressureSource, Surface pressureDestination);
    void applyPressure(Surface pressureSurface, Surface velocityDestination);

};

#endif
