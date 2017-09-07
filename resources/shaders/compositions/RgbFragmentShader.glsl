#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D textureA;
uniform sampler2D textureB;

void main() {
    vec3 a = texture(textureA, UV).rgb / 5.0f;

    color = vec4(a.r, a.g, a.b, 1.0f);
}
