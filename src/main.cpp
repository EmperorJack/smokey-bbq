#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <smoke_simulation.hpp>
#include <audio_analyzer.hpp>

// Object variables
SmokeSimulation* smokeSimulation = nullptr;
AudioAnalyzer* audioAnalyzer = nullptr;

// Mouse variables
glm::vec2 mousePosition;
bool mousePressed = false;

// Toggles
bool updateSmokeSimulation = false;
bool displayDensityField = false;
bool displayVelocityField = false;
bool updateAudioData = true;
bool displayAudioData = true;
bool smokeAudio = false;

// Mouse Position callback
void mouseMovedCallback(GLFWwindow* win, double xPos, double yPos) {
    mousePosition = glm::vec2(xPos, yPos);
}

// Mouse Button callback
void mouseButtonCallback(GLFWwindow *win, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) mousePressed = true;
        else if (action == GLFW_RELEASE) mousePressed = false;
    }
}

// Keyboard callback
void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    if (key == ' ' && action == GLFW_PRESS) {
        smokeSimulation->resetFields();
        audioAnalyzer->resetBuffers();
    } else if (key == 'E' && action == GLFW_PRESS) {
        smokeSimulation->enableEmitter = !smokeSimulation->enableEmitter;
    } else if (key == 'P' && action == GLFW_PRESS) {
        smokeSimulation->enablePressureSolve = !smokeSimulation->enablePressureSolve;
    } else if (key == 'R' && action == GLFW_PRESS) {
        smokeSimulation->randomPulseAngle = !smokeSimulation->randomPulseAngle;
    } else if (key == 'W' && action == GLFW_PRESS) {
        smokeSimulation->wrapBorders = !smokeSimulation->wrapBorders;
    } else if (key == 'B' && action == GLFW_PRESS) {
        smokeSimulation->enableBuoyancy = !smokeSimulation->enableBuoyancy;
    } else if (key == 'S' && action == GLFW_PRESS) {
        displayDensityField = !displayDensityField;
    } else if (key == 'V' && action == GLFW_PRESS) {
        displayVelocityField = !displayVelocityField;
    } else if (key == 'U' && action == GLFW_PRESS) {
        updateSmokeSimulation = !updateSmokeSimulation;
    } else if (key == 'A' && action == GLFW_PRESS) {
        displayAudioData = !displayAudioData;
    } else if (key == 'F' && action == GLFW_PRESS) {
        updateAudioData = !updateAudioData;
    } else if (key == 'Z' && action == GLFW_PRESS) {
        smokeAudio = !smokeAudio;
    }
}

int main(int argc, char **argv) {

    // Initialise GLFW
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Open a window and create its OpenGL context
    GLFWwindow* window;
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window", (FULL_SCREEN ? glfwGetPrimaryMonitor() : NULL), NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = 1; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Input callbacks
    glfwSetCursorPosCallback(window, mouseMovedCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    // Setup the vertex array object
    GLuint vertexArray;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    printf("\n~~~\n\n");

    // Setup objects
    smokeSimulation = new SmokeSimulation();
    audioAnalyzer = new AudioAnalyzer();

    // printf("\n~~~\n\n");
    // audioAnalyzer->printAudioDevices();
    printf("\n~~~\n\n");

    double lastTime = glfwGetTime();
    int frameCount = 0;

    // Check if the escape key was pressed or the window was closed
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {

        // Measure speed
        double currentTime = glfwGetTime();
        frameCount++;

        // Print the frame time every second
        if (currentTime - lastTime >= 1.0) {
            printf("%f ms/frame\n", 1000.0 / (double) frameCount);
            frameCount = 0;
            lastTime += 1.0;
        }

        if (smokeAudio) {
            float sideOffset = ((float) SCREEN_WIDTH * 0.08f);
            float bandSpacing = ((float) SCREEN_WIDTH - sideOffset * 2.0f) / (AudioAnalyzer::NUM_BANDS * 2);

            for (int i = 0; i < AudioAnalyzer::NUM_BANDS * 2; i++) {
                int index;
                if (i < AudioAnalyzer::NUM_BANDS) {
                    index = AudioAnalyzer::NUM_BANDS - i - 1;
                } else {
                    index = i % AudioAnalyzer::NUM_BANDS;
                }

                float value = audioAnalyzer->getFrequencyBand(index);

                if (value < 3.0f) continue;

                value = min(value, 30.0f);

                glm::vec2 position = vec2(i * bandSpacing + sideOffset, SCREEN_HEIGHT * 0.95f);
                glm::vec2 force = vec2(myRandom() * 100.0f - 50.0f, (value + 0.5f) * -10.0f);

                smokeSimulation->emit(position, force, bandSpacing * 0.9f, value * 0.005f, value * 0.02f);
            }
        }

        if (mousePressed) smokeSimulation->addPulse(mousePosition);

        if (updateSmokeSimulation) smokeSimulation->update();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::ortho(0.0f, (float) SCREEN_WIDTH, (float) SCREEN_HEIGHT, 0.0f);
        glm::mat4 view = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));

        glm::mat4 mvp = projection * view;

        if (displayDensityField) smokeSimulation->renderDensity();
        if (displayVelocityField) smokeSimulation->renderVelocityField(mvp, mousePosition);

        if (updateAudioData) audioAnalyzer->update();

        if (displayAudioData) audioAnalyzer->renderWaveform(mvp);
        // if (displayAudioData) audioAnalyzer->renderLinearSpectrum(mvp);
        if (displayAudioData) audioAnalyzer->renderLogSpectrum(mvp);
        if (displayAudioData) audioAnalyzer->renderFrequencyBands(mvp);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    audioAnalyzer->shutDown();

    glfwTerminate();
    return 0;
}
