#version 330 core
layout(location = 0) in vec3 vertexPosition_modelspace;

out vec2 UV;

void main() {
    gl_Position = vec4(vertexPosition_modelspace, 1);

    UV = vec2(vertexPosition_modelspace.x, -vertexPosition_modelspace.y) * 0.5f + 0.5f;
}