#include <iostream>
#include <main.hpp>
#include <opengl.hpp>
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <smoke_simulation/smoke_simulation.hpp>
#include <audio_analyzer/audio_analyzer.hpp>
#include <compositions/composition.hpp>
#include <compositions/horizontal_spectrum.hpp>
#include <smoke_simulation/smoke_simulation_gui.hpp>
#include <audio_analyzer/audio_analyzer_gui.hpp>

// Components
SmokeSimulation* smokeSimulation = nullptr;
AudioAnalyzer* audioAnalyzer = nullptr;

// Compositions
int currentComposition;
std::vector<Composition*> compositions;

// Gui instances
SmokeSimulationGui* smokeSimulationGui = nullptr;
AudioAnalyzerGui* audioAnalyzerGui = nullptr;

// Mouse variables
glm::vec2 mousePosition;
bool mousePressed = false;

// Toggles
bool displaySmokeSimulationGui = false;
bool displayAudioAnalyzerGui = false;
bool smokeAudio = false;
bool printFrameTimes = false;

void mouseMovedCallback(GLFWwindow* win, double xPos, double yPos) {
    mousePosition = glm::vec2(xPos, yPos);
}

void mouseButtonCallback(GLFWwindow *win, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) mousePressed = true;
        else if (action == GLFW_RELEASE) mousePressed = false;
    }
}

void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    if (key == ' ' && action == GLFW_PRESS) {
        smokeSimulation->reset();
        audioAnalyzer->resetBuffers();
    } else if (key == 'G' && action == GLFW_PRESS) {
        smokeSimulation->useGPUImplementation = !smokeSimulation->useGPUImplementation;
    } else if (key == 'S' && action == GLFW_PRESS) {
        displaySmokeSimulationGui = !displaySmokeSimulationGui;
    } else if (key == 'A' && action == GLFW_PRESS) {
        displayAudioAnalyzerGui = !displayAudioAnalyzerGui;
    } else if (key == 'Z' && action == GLFW_PRESS) {
        smokeAudio = !smokeAudio;
    } else if (key == 'F' && action == GLFW_PRESS) {
        printFrameTimes = !printFrameTimes;
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
    glfwWindowHint(GLFW_DECORATED, BORDERLESS ? GL_FALSE : GL_TRUE);
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

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, false);

    printf("\n~~~\n\n");

    // Setup object instances
    smokeSimulation = new SmokeSimulation();
    audioAnalyzer = new AudioAnalyzer();

    // Setup compositions
    currentComposition = 0;
    compositions.push_back(new HorizontalSpectrum(smokeSimulation, audioAnalyzer, "compositions/SmokeFragmentShader"));

    // Setup GUI instances
    smokeSimulationGui = new SmokeSimulationGui(smokeSimulation);
    audioAnalyzerGui = new AudioAnalyzerGui(audioAnalyzer);

    printf("\n~~~\n\n");

    double lastTime = glfwGetTime();
    int frameCount = 0;

    // Check if the escape key was pressed or the window was closed
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {

        // Poll events and setup GUI for the current frame
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        // Measure speed
        double currentTime = glfwGetTime();
        frameCount++;

        // Print the frame time every second
        if (currentTime - lastTime >= 1.0) {
            if (printFrameTimes) printf("%f ms/frame\n", 1000.0 / (double) frameCount);
            frameCount = 0;
            lastTime += 1.0;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (smokeAudio) {
            compositions[currentComposition]->update();
        }

        if (mousePressed && !ImGui::IsMouseHoveringAnyWindow()) smokeSimulation->addPulse(mousePosition);

        smokeSimulation->update();

        glm::mat4 projection = glm::ortho(0.0f, (float) SCREEN_WIDTH, (float) SCREEN_HEIGHT, 0.0f);
        glm::mat4 view = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));

        glm::mat4 mvp = projection * view;

        smokeSimulation->render(mvp, mousePosition);

        audioAnalyzer->update();

        audioAnalyzer->render(mvp);

        // Render GUI
        if (displaySmokeSimulationGui) smokeSimulationGui->render();
        if (displayAudioAnalyzerGui) audioAnalyzerGui->render();
        ImGui::Render();

        glfwSwapBuffers(window);
    }

    audioAnalyzer->shutDown();
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    delete smokeSimulation;
    delete audioAnalyzer;
    for (Composition* composition : compositions) {
        delete composition;
    }
    delete smokeSimulationGui;
    delete audioAnalyzerGui;

    return 0;
}
