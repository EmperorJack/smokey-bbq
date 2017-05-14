#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <iostream>

bool displayVectors = false;
bool displayDensity = true;
bool enableEmitter = false;
bool enablePressureSolve = true;

GLuint squareVBO;
GLuint velocityVBO;
GLuint densityVBO;
GLuint densityUVVBO;
GLuint densityTextureID;

float gridWorldSize;
float gridSpacing;

SmokeSimulation::gridCell grid[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];

static inline float myRandom() {
    return std::rand() % 100 / 100.0f;
}

static inline int intFloor(float x) {
    int i = (int) x; /* truncate */
    return i - (i > x); /* convert trunc to floor */
}

SmokeSimulation::SmokeSimulation(float _gridWorldSize) {
    gridWorldSize = _gridWorldSize;
    gridSpacing = gridWorldSize / GRID_SIZE;
    setupFields();

    // Setup VBOs
    float squareVertices[] = {
            0.0f, 0.0f,
            0.0f, gridSpacing,
            gridSpacing, gridSpacing,
            gridSpacing, 0.0f,
    };
    glGenBuffers(1, &squareVBO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    float vectorVertices[] = {
            -STROKE_WEIGHT / 2.0f, 0.0f,
            -STROKE_WEIGHT / 2.0f, gridSpacing / 2.5f,
            STROKE_WEIGHT / 2.0f, gridSpacing / 2.5f,
            STROKE_WEIGHT / 2.0f, 0.0f,
    };
    glGenBuffers(1, &velocityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vectorVertices), vectorVertices, GL_STATIC_DRAW);

    float densityVertices[] = {
            -gridWorldSize / 2.0f, -gridWorldSize / 2.0f,
            -gridWorldSize / 2.0f, gridWorldSize / 2.0f,
            gridWorldSize / 2.0f, gridWorldSize / 2.0f,
            gridWorldSize / 2.0f, -gridWorldSize / 2.0f,
    };
    glGenBuffers(1, &densityVBO);
    glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(densityVertices), densityVertices, GL_STATIC_DRAW);

    float densityUVs[] = {
            0.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
    };
    glGenBuffers(1, &densityUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, densityUVVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(densityUVs), densityUVs, GL_STATIC_DRAW);

    // Setup density texture
    glGenTextures(1, &densityTextureID);

    glBindTexture(GL_TEXTURE_2D, densityTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void SmokeSimulation::setupFields() {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            gridCell g;

            // Setup velocity
            float x = i * gridSpacing + gridSpacing / 2.0f;
            float y = j * gridSpacing + gridSpacing / 2.0f;
            g.velocity =
                    //glm::normalize
            (glm::vec2(
                    //sin(2 * M_PI * y), sin(2 * M_PI * x)
                    //y, x
                    //-0.01f, -0.01f
                    0.0f, 0.0f
                    //myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f
                    //1.0f, sin(2 * M_PI * y)
            ));

            // Setup density
            g.density = 0.0f;

            grid[i][j] = g;
        }
    }
}

void SmokeSimulation::update() {
    // Advect velocity through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].advectedVelocity = advectedVelocityAt(i, j);
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].velocity = grid[i][j].advectedVelocity;
        }
    }

    // Smoke emitter
    if (enableEmitter) {
        glm::vec2 target = glm::vec2(GRID_SIZE / 2 * gridSpacing, GRID_SIZE * gridSpacing - 2);

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                float x = i * gridSpacing;
                float y = j * gridSpacing;
                glm::vec2 gridPosition = glm::vec2(x, y);
                if (glm::distance(target, gridPosition) < PULSE_RANGE / 3.0f) {
                    float horizontalForce = PULSE_FORCE * 50;
                    grid[i][j].velocity = glm::vec2(myRandom() * PULSE_FORCE - PULSE_FORCE / 2.0f, -PULSE_FORCE);
                    grid[i][j].density += 0.2f;
                }
            }
        }
    }

    // Compute divergence
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].divergence = divergenceAt(i, j);
        }
    }

    // Solve and apply pressure
    solvePressureField();
    if (enablePressureSolve) applyPressure();

    // Advect density through velocity
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].advectedDensity = advectedDensityAt(i, j) * DENSITY_DISSAPATION;
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].density = grid[i][j].advectedDensity;
        }
    }
}

glm::vec2 SmokeSimulation::advectedVelocityAt(int i, int j) {
    glm::vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing);

    return getVelocity(tracePosition.x, tracePosition.y);
}

float SmokeSimulation::advectedDensityAt(int i, int j) {
    glm::vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing);

    return getDensity(tracePosition.x, tracePosition.y);
}

glm::vec2 SmokeSimulation::traceParticle(float x, float y) {
    //return glm::vec2(x, y) - (TIME_STEP * getVelocity(x, y));
    glm::vec2 v = getVelocity(x, y);
    v = getVelocity(x + 0.5f * TIME_STEP * v.x, y + 0.5f * TIME_STEP * v.y);
    return glm::vec2(x, y) - (TIME_STEP * v);

    // Euler method:         y = x + ∆t * u(x)
    // Runge kutta 2 method: y = x + ∆t * u(x + ∆t / 2 * u(x))
}

float SmokeSimulation::divergenceAt(int i, int j) {
    float a = -((2 * gridSpacing * FLUID_DENSITY) / TIME_STEP);

    float b = getVelocity((i + 1) * gridSpacing, j * gridSpacing).x -
              getVelocity((i - 1) * gridSpacing, j * gridSpacing).x +
              getVelocity(i * gridSpacing, (j + 1) * gridSpacing).y -
              getVelocity(i * gridSpacing, (j - 1) * gridSpacing).y;

    return a * b;
}

void SmokeSimulation::solvePressureField() {
    // Reset the pressure field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].pressure = 0.0f;
        }
    }

    // Iteratively solve the new pressure field
    for (int iteration = 0; iteration < JACOBI_ITERATIONS; iteration++) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                grid[i][j].newPressure = pressureAt(i, j);
            }
        }

        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                grid[i][j].pressure = grid[i][j].newPressure;
            }
        }
    }
}

float SmokeSimulation::pressureAt(int i, int j) {
    float d = grid[i][j].divergence;
    float p = grid[clampIndex(i + 2)][j].pressure +
              grid[clampIndex(i - 2)][j].pressure +
              grid[i][clampIndex(j + 2)].pressure +
              grid[i][clampIndex(j - 2)].pressure;
    return (d + p) / 4.0f;
}

void SmokeSimulation::applyPressure() {
    float a = -(TIME_STEP / (2 * FLUID_DENSITY * gridSpacing));

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float xChange = grid[clampIndex(i + 1)][j].pressure - grid[clampIndex(i - 1)][j].pressure;
            float yChange = grid[i][clampIndex(j + 1)].pressure - grid[i][clampIndex(j - 1)].pressure;

            grid[i][j].velocity.x += a * xChange;
            grid[i][j].velocity.y += a * yChange;
        }
    }
}

glm::vec2 SmokeSimulation::getVelocity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    glm::vec2 v = glm::vec2();

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedVelocity(normX - 0.5f, normY, 0) +
           getInterpolatedVelocity(normX + 0.5f, normY, 0)) / 2.0f;
    v.y = (getInterpolatedVelocity(normX, normY - 0.5f, 1) +
           getInterpolatedVelocity(normX, normY + 0.5f, 1)) / 2.0f;

    //if (v.x == 0.0f && v.y == 0.0f) std::cout << "YO NAN " << std::endl;

    return v;
}

float SmokeSimulation::getDensity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedDensity(normX, normY);
}

float SmokeSimulation::getInterpolatedVelocity(float x, float y, int index) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getGridVelocity(i, j)[index] +
           (x-i) * (j+1-y)   * getGridVelocity(i+1, j)[index] +
           (i+1-x) * (y-j)   * getGridVelocity(i, j+1)[index] +
           (x-i) * (y-j)     * getGridVelocity(i+1, j+1)[index];
}

glm::vec2 SmokeSimulation::getGridVelocity(int i, int j) {
    return grid[clampIndex(i)][clampIndex(j)].velocity;
}

float SmokeSimulation::getInterpolatedDensity(float x, float y) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getGridDensity(i, j) +
           (x-i) * (j+1-y)   * getGridDensity(i+1, j) +
           (i+1-x) * (y-j)   * getGridDensity(i, j+1) +
           (x-i) * (y-j)     * getGridDensity(i+1, j+1);
}

float SmokeSimulation::getGridDensity(int i, int j) {
    return grid[clampIndex(i)][clampIndex(j)].density;
}

int SmokeSimulation::clampIndex(int i) {
    if (WRAP_BORDERS) {
        if (i < 0) i = GRID_SIZE + (i % GRID_SIZE);
        else i = i % GRID_SIZE;
    } else {
        i = max(i, 0); i = min(i, GRID_SIZE - 1);
    }

    return i;
}

void SmokeSimulation::addPulse(glm::vec2 position) {
    position -= glm::vec2(gridSpacing / 2.0f, gridSpacing / 2.0f);

    glm::vec2 force = glm::vec2(myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::vec2 gridPosition = glm::vec2(x, y);
            if (glm::distance(position, gridPosition) < PULSE_RANGE) {
                grid[i][j].velocity = PULSE_FORCE * glm::normalize(glm::vec2(force)); // gridPosition - position // 0.0f, -1.0f
                grid[i][j].density += 0.5f * (1 - glm::distance(position, gridPosition) / PULSE_RANGE);
            }
        }
    }
}

void SmokeSimulation::render(glm::mat4 transform, glm::vec2 mousePosition) {
    if (displayDensity) {
        glm::mat4 densityTransform = glm::mat4(transform);
        densityTransform = glm::translate(densityTransform, glm::vec3(gridWorldSize / 2.0f, gridWorldSize / 2.0f, 0.0f));
        densityTransform = glm::scale(densityTransform, glm::vec3(-1.0f, 1.0f, 1.0f));
        densityTransform = glm::rotate(densityTransform, (float) M_PI / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        drawDensity(densityTransform);
    }

    if (!displayVectors) return;

    float velocityColor[] = {0.0f, 0.0f, 1.0f, 0.0f};
    setColor(velocityColor);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing;
            float y = j * gridSpacing;
            glm::mat4 translate = glm::translate(glm::vec3(x, y, 0.0f));

            translate *= glm::translate(glm::vec3(gridSpacing / 2.0f, gridSpacing / 2.0f, 0.0f));

            glm::vec3 velocity = glm::vec3(grid[i][j].velocity.x, grid[i][j].velocity.y, 0.0f);

            float magnitude = max(min(glm::length(velocity) / PULSE_FORCE * 1.5f, 2.0f), 0.5f);

            glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
            glm::mat4 rotate = glm::orientation(glm::normalize(velocity), glm::vec3(0.0f, 1.0f, 0.0f));

            drawLine(transform * translate * scale * rotate);
        }
    }

    // Draw interpolated velocity and mouse position
    glm::mat4 translate = glm::translate(glm::vec3(mousePosition, 0.0f));

    glm::vec2 velocity = getVelocity(mousePosition.x - gridSpacing / 2.0f, mousePosition.y - gridSpacing / 2.0f);

    float magnitude = min(glm::length(velocity), 2.0f);
    glm::mat4 scale = glm::scale(glm::vec3(magnitude, magnitude, 1.0f));
    glm::mat4 rotate = glm::orientation(glm::vec3(glm::normalize(velocity), 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    float mouseColor[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    setColor(mouseColor);
    drawLine(transform * translate * scale * rotate);
}

void SmokeSimulation::drawDensity(glm::mat4 transform) {
    GLint useFillColorLocation = glGetUniformLocation(3, "useFillColor");
    glUniform1i(useFillColorLocation, false);

    float densityField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            densityField[i][j] = grid[i][j].density;
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, &densityField[0][0]);

    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    // Bind vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, densityVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    // Bind uvs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, densityUVVBO);
    glVertexAttribPointer(
            1,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void SmokeSimulation::drawSquare(glm::mat4 transform, bool fill) {
    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    if (fill) {
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else {
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    glDisableVertexAttribArray(0);
}

void SmokeSimulation::drawLine(glm::mat4 transform) {
    GLint transformID = glGetUniformLocation(3, "MVP");
    glUniformMatrix4fv(transformID, 1, GL_FALSE, &transform[0][0]);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glVertexAttribPointer(
            0,         // shader layout attribute
            2,         // size
            GL_FLOAT,  // type
            GL_FALSE,  // normalized?
            0,         // stride
            (void*)0   // array buffer offset
    );

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableVertexAttribArray(0);
}

void SmokeSimulation::toggleVectorDisplay() {
    displayVectors = !displayVectors;
}

void SmokeSimulation::toggleDensityDisplay() {
    displayDensity = !displayDensity;
}

void SmokeSimulation::toggleEnableEmitter() {
    enableEmitter = !enableEmitter;
}

void SmokeSimulation::togglePressureSolve() {
    enablePressureSolve = !enablePressureSolve;
}
