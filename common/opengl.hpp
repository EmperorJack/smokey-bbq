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

static inline void setColor(int shader, float color[]) {
    GLint useFillColorLocation = glGetUniformLocation(shader, "useFillColor");
    glUniform1i(useFillColorLocation, true);

    GLint fillColorLocation = glGetUniformLocation(shader, "fillColor");
    glUniform4fv(fillColorLocation, 1, color);
}

static inline void passScreenSize(int shader) {
    GLint screenWidthLocation = glGetUniformLocation(shader, "screenWidth");
    glUniform1i(screenWidthLocation, SCREEN_WIDTH);

    GLint screenHeightLocation = glGetUniformLocation(shader, "screenHeight");
    glUniform1i(screenHeightLocation, SCREEN_HEIGHT);
}

#endif
