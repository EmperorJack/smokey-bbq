#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D temperatureTexture;

void main() {
    float density = texture(temperatureTexture, UV).r;
    density = 1.0f - clamp(density, 0.0, 1.0);

    color = vec4(density, density, density, 1.0);
}