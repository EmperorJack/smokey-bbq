#include <iostream>

#include <opengl.hpp>
#include <shaderLoader.hpp>
#include <smoke_simulation.hpp>

SmokeSimulation* smokeSimulation = nullptr;

const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT= 1000;

glm::vec2 mousePosition;

// Mouse Position callback
void mouseMovedCallback(GLFWwindow* win, double xPos, double yPos) {
    mousePosition = glm::vec2(xPos, yPos);
}

// Mouse Button callback
void mouseButtonCallback(GLFWwindow *win, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        smokeSimulation->addPulse(mousePosition);
    }
}

// Keyboard callback
void keyCallback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    if (key == ' ') {
        smokeSimulation->setupFields();
    } else if (key == 'V' && action == GLFW_PRESS) {
        smokeSimulation->toggleVectorDisplay();
    } else if (key == 'S' && action == GLFW_PRESS) {
        smokeSimulation->toggleDensityDisplay();
    }
}

int main(int argc, char **argv) {

    // Initialise GLFW
    if(!glfwInit()) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

    // Open a window and create its OpenGL context
    GLFWwindow* window;
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
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

    // Create and compile our GLSL program from the shaders
    GLuint programID = loadShaders("resources/shaders/SimpleVertexShader.vs", "resources/shaders/SimpleFragmentShader.fs");

    // Setup the Vertex Array Object
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    int frameCount = 0;
    smokeSimulation = new SmokeSimulation(SCREEN_WIDTH);

    // Check if the ESC key was pressed or the window was closed
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {

        smokeSimulation->update();

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        glm::mat4 projection = glm::ortho(0.0f, (float) SCREEN_WIDTH, (float) SCREEN_HEIGHT, 0.0f);
        glm::mat4 view = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)); // SCREEN_WIDTH / 2, SCREEN_HEIGHT/ 2

        glm::mat4 mvp = projection * view;

        smokeSimulation->render(mvp, mousePosition);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        frameCount++;
    }

    glfwTerminate();
    return 0;
}
