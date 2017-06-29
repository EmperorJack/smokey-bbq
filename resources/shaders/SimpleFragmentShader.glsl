#version 330 core

layout(location = 0) out vec3 color;

uniform vec4 fillColor = vec4(0.0, 0.0, 0.0, 0.0);

void main() {
    color = vec3(1.0f, 0.0f, 0.0f);
}
