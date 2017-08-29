#version 330 core

layout(location = 0) out vec4 color;

uniform vec2 position;
uniform float radius;
uniform vec3 fill;

void main() {
    float distance = distance(position, gl_FragCoord.xy);

    if (distance < radius) {
        color = vec4(fill, 1.0f);
    } else {
        color = vec4(0.0f);
    }
}
