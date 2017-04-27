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
using namespace glm;

inline void setColor(float color[]) {
    GLint cLoc = glGetUniformLocation(3, "fillColor");
    glProgramUniform4fv(3, cLoc, GL_TRUE, color);
}

#endif
