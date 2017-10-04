//
// Created by Jack Purvis
//

#ifndef OPENGL_H
#define OPENGL_H

#include <main.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>

using namespace glm;

extern GLFWwindow* window;

static inline void setColor(int shader, float color[]) {
    GLint useFillColorLocation = glGetUniformLocation(shader, "useFillColor");
    glUniform1i(useFillColorLocation, true);

    GLint fillColorLocation = glGetUniformLocation(shader, "fillColor");
    glUniform4fv(fillColorLocation, 1, color);
}

static inline void resetViewportToFramebuffer() {
    if (window == NULL) return;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
}

#endif
