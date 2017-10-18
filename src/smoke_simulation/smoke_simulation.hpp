#ifndef SMOKE_SIMULATION_HPP
#define SMOKE_SIMULATION_HPP

#include <map>
#include <opengl.hpp>

class SmokeSimulation {

public:

    // Constants
    static constexpr int GRID_SIZE = 512;
    static constexpr int BENCHMARK_SAMPLES = 60;

    // Variables
    float timeStep;
    float fluidDensity;
    int jacobiIterations;

    float gravity;
    float pulseRange;
    float emitterRange;
    float pulseForce;

    float velocityDissipation;
    float densityDissipation;
    float temperatureDissipation;
    float rgbDissipation;

    float riseForce;
    float fallForce;
    float atmosphereTemperature;

    float strokeWeight;

    float vorticityConfinementForce;

    // Benchmarking variables
    bool benchmarking;
    int benchmarkSample;
    std::vector<double> updateTimes;

    // Setup
    SmokeSimulation();
    void setDefaultVariables();
    void setDefaultToggles();
    void reset();

    // Display toggle
    enum Display {
        COMPOSITION,
        DENSITY,
        VELOCITY,
        TEMPERATURE,
        CURL,
        RGB
    };
    Display currentDisplay;

    // Updating
    void update();
    void setCompositionData(GLuint shader, std::vector<Display> fields);

    // Benchmarking
    void beginBenchmark();
    void finishBenchmark();

    // Rendering
    void render(glm::mat4 transform, glm::vec2 mousePosition);

    // Interactions
    void addPulse(glm::vec2);
    void emit(glm::vec2 position, float range, std::vector<Display> fields, std::vector<glm::vec3> values);

    // Toggle variables
    bool displaySmokeField;
    bool displayVelocityField;
    bool updateSimulation;
    bool enableEmitter;
    bool enablePressureSolver;
    bool randomPulseAngle;
    bool enableBuoyancy;
    bool wrapBorders, prevWrapBorders;
    bool enableVorticityConfinement;
    bool computeIntermediateFields;
    bool useCPUMultithreading;
    bool useGPUImplementation;

private:

    // Instance variables
    float gridSpacing;

    // Vertex buffer objects
    GLuint lineVBO;
    GLuint fullscreenVBO;

    // Shaders
    GLuint simpleShader;
    GLuint compositionShader;
    std::vector<Display> compositionFields;
    std::map<Display, GLuint> fieldShaders;

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
    glm::vec3 dataForDisplayCPU(Display display, int i, int j);

    // Interactions
    void emitCPU(glm::vec2 position, float range, std::vector<Display> fields, std::vector<glm::vec3> values);

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
    glm::vec3 rgb[GRID_SIZE][GRID_SIZE];
    glm::vec3 advectedRgb[GRID_SIZE][GRID_SIZE];

    // Rendering fields and textures
    glm::vec3 textureFieldA[GRID_SIZE][GRID_SIZE];
    glm::vec3 textureFieldB[GRID_SIZE][GRID_SIZE];
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
    glm::vec3 getRgb(float x, float y);

    // Field interpolation
    float getInterpolatedVelocity(float x, float y, bool xAxis);
    float getInterpolatedDensity(float x, float y);
    float getInterpolatedTemperature(float x, float y);
    glm::vec3 getInterpolatedRgb(float x, float y);

    // Grid access
    glm::vec2 getGridVelocity(int i, int j);
    float getGridDensity(int i, int j);
    float getGridTemperature(int i, int j);
    float getGridPressure(int i, int j);
    float getGridCurl(int i, int j);
    glm::vec3 getGridRgb(int i, int j);

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
    std::vector<Slab> slabs;
    Slab velocitySlab;
    Slab densitySlab;
    Slab temperatureSlab;
    Slab curlSlab;
    Slab divergenceSlab;
    Slab pressureSlab;
    Slab rgbSlab;

    // Samplers
    GLuint boundedSampler;
    GLuint wrapBordersSampler;

    // Setup
    void initGPU();
    void initPrograms();
    void initSlabs();
    Slab createSlab(int width, int height, int numComponents);
    Surface createSurface(int width, int height, int numComponents);
    void updateSampler();

    // Core
    void updateGPU();
    void renderGPU();
    Slab dataForDisplayGPU(Display display);

    // Interactions
    void emitGPU(glm::vec2 position, float range, std::vector<Display> fields, std::vector<glm::vec3> values);

    // State functions
    void swapSurfaces(Slab &slab);
    void clearSurface(Surface s, float v);
    void resetSlabs();
    void resetState();

    // Algorithm
    void advect(Surface velocitySurface, Surface source, Surface destination, float dissipation);
    void applyImpulse(Surface destination, glm::vec2 position, float radius, glm::vec3 fill, bool allowOutwardImpulse);
    void applyBuoyancy(Surface temperatureSurface, Surface densitySurface, Surface velocityDestination);
    void computeCurl(Surface velocitySurface, Surface curlSurface);
    void applyVorticityConfinement(Surface curlSurface, Surface velocityDestination);
    void computeDivergence(Surface velocitySurface, Surface divergenceSurface);
    void jacobi(Surface divergenceSurface, Surface pressureSource, Surface pressureDestination);
    void applyPressure(Surface pressureSurface, Surface velocityDestination);

};

#endif
