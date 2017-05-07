#ifndef OPENGL_H
#define OPENGL_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
using namespace glm;

inline void setColor(float color[]) {
    GLint useFillColorLocation = glGetUniformLocation(3, "useFillColor");
    glUniform1i(useFillColorLocation, true);

    GLint fillColorLocation = glGetUniformLocation(3, "fillColor");
    glUniform4fv(fillColorLocation, 1, color);
}

#endif
