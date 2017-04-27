#include <iostream>
#include <math.h>

#include <opengl.hpp>
#include <shaderLoader.hpp>
#include <smoke_simulation.hpp>

SmokeSimulation* smokeSimulation = nullptr;

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
    window = glfwCreateWindow( 1024, 768, "Window", NULL, NULL);
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

    // Create and compile our GLSL program from the shaders
    GLuint programID = loadShaders("resources/shaders/SimpleVertexShader.vs", "resources/shaders/SimpleFragmentShader.fs");

    // Setup the Vertex Array Object
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    int frameCount = 0;

    smokeSimulation = new SmokeSimulation();

    // Check if the ESC key was pressed or the window was closed
    while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {

        smokeSimulation->update();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);

        smokeSimulation->render();

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        frameCount++;
    }

    glfwTerminate();
    return 0;
}