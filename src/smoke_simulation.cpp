#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <iostream>

bool displayVectors = false;
bool displayDensity = true;
bool enableEmitter = false;

glm::vec2 velocityField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];
glm::vec2 densityField[SmokeSimulation::GRID_SIZE][SmokeSimulation::GRID_SIZE];

GLuint squareVBO;
GLuint velocityVBO;
GLuint densityVBO;
GLuint densityUVVBO;

float gridWorldSize;
float gridSpacing;

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
    GLuint textureID;
    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void SmokeSimulation::setupFields() {
    // Setup velocity field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            float x = i * gridSpacing + gridSpacing / 2.0f;
            float y = j * gridSpacing + gridSpacing / 2.0f;
            velocityField[i][j] =
                    //glm::normalize
            (glm::vec2(
                    //sin(2 * M_PI * y), sin(2 * M_PI * x)
                    //y, x
                    //-0.01f, -0.01f
                    0.0f, 0.0f
                    //myRandom() * 2.0f - 1.0f, myRandom() * 2.0f - 1.0f
                    //1.0f, sin(2 * M_PI * y)
            ));
        }
    }

    // Setup density field
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            densityField[i][j] = glm::vec2(0.0f, 0.0f);
        }
    }
}

void SmokeSimulation::update() {
    glm::vec2 advectedVelocityField[GRID_SIZE][GRID_SIZE];
    glm::vec2 advectedDensityField[GRID_SIZE][GRID_SIZE];

    glm::vec2 target = glm::vec2(GRID_SIZE / 2 * gridSpacing, GRID_SIZE * gridSpacing);

    // Smoke emitter
    if (enableEmitter) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                float x = i * gridSpacing;
                float y = j * gridSpacing;
                glm::vec2 gridPosition = glm::vec2(x, y);
                if (glm::distance(target, gridPosition) < PULSE_RANGE / 3.0f) {
                    velocityField[i][j] = glm::vec2(myRandom() * 4.0f - 2.0f, -PULSE_FORCE);
                    densityField[i][j] += 0.3f;
                }
            }
        }
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedVelocityField[i][j] = advectedVelocityAt(i, j);
        }
    }

    std::copy(&advectedVelocityField[0][0], &advectedVelocityField[0][0] + GRID_SIZE * GRID_SIZE, &velocityField[0][0]);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            advectedDensityField[i][j] = glm::vec2(advectedDensityAt(i, j) * DENSITY_DISSAPATION, 0.0f);
        }
    }

    std::copy(&advectedDensityField[0][0], &advectedDensityField[0][0] + GRID_SIZE * GRID_SIZE, &densityField[0][0]);
}

glm::vec2 SmokeSimulation::advectedVelocityAt(int i, int j) {
    glm::vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing, TIME_STEP);

    return getVelocity(tracePosition.x, tracePosition.y);
}

float SmokeSimulation::advectedDensityAt(int i, int j) {
    glm::vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing, TIME_STEP);

    return getDensity(tracePosition.x, tracePosition.y);
}


glm::vec2 SmokeSimulation::traceParticle(float x, float y, float timeStep) {
    //return glm::vec2(x, y) - (timeStep * getVelocity(x, y));
    glm::vec2 v = getVelocity(x, y);
    v = getVelocity(x + 0.5f * timeStep * v.x, y + 0.5f * timeStep * v.y);
    return glm::vec2(x, y) - (timeStep * v);

    // Euler method:         y = x + ∆t * u(x)
    // Ranga kutta 2 method: y = x + ∆t * u(x + ∆t / 2 * u(x))
}

glm::vec2 SmokeSimulation::getVelocity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    glm::vec2 v = glm::vec2();

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedValue(velocityField, normX - 0.5f, normY, 0) +
           getInterpolatedValue(velocityField, normX + 0.5f, normY, 0)) / 2.0f;
    v.y = (getInterpolatedValue(velocityField, normX, normY - 0.5f, 1) +
           getInterpolatedValue(velocityField, normX, normY + 0.5f, 1)) / 2.0f;

    //if (v.x == 0.0f && v.y == 0.0f) std::cout << "YO NAN " << std::endl;

    return v;
}

float SmokeSimulation::getDensity(float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    return getInterpolatedValue(densityField, normX, normY, 0);
}

float SmokeSimulation::getInterpolatedValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], float x, float y, int index) {
    int i = (int) floor(x);
    int j = (int) floor(y);

    return (i+1-x) * (j+1-y) * getGridValue(field, i, j)[index] +
           (x-i) * (j+1-y)   * getGridValue(field, i+1, j)[index] +
           (i+1-x) * (y-j)   * getGridValue(field, i, j+1)[index] +
           (x-i) * (y-j)     * getGridValue(field, i+1, j+1)[index];
}

glm::vec2 SmokeSimulation::getGridValue(glm::vec2 field[GRID_SIZE][GRID_SIZE], int i, int j) {
    if (WRAP_BORDERS) {
        if (i < 0) i = GRID_SIZE - 1;
        else if (i >= GRID_SIZE) i = 0;
        if (j < 0) j = GRID_SIZE - 1;
        else if (j >= GRID_SIZE) j = 0;
    } else {
        i = max(i, 0); i = min(i, GRID_SIZE - 1);
        j = max(j, 0); j = min(j, GRID_SIZE - 1);
    }

    //if (field[i][j] != field[i][j]) std::cout << "YO NAN" << std::endl;

    return field[i][j];
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
                velocityField[i][j] = PULSE_FORCE * glm::normalize(glm::vec2(force)); // gridPosition - position // 0.0f, -1.0f
                densityField[i][j] += 0.5f / (glm::distance(position, gridPosition) / PULSE_RANGE);
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

            glm::vec3 velocity = glm::vec3(velocityField[i][j].x, velocityField[i][j].y, 0.0f);

            float magnitude = max(min(glm::length(velocity), 2.0f), 0.5f);

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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, GRID_SIZE, GRID_SIZE, 0, GL_RG, GL_FLOAT, &densityField[0][0]);

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

float SmokeSimulation::myRandom() {
    return std::rand() % 100 / 100.0f;
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
