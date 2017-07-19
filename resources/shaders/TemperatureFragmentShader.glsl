#version 330 core

in vec2 UV;

out vec3 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D temperatureTexture;

void main() {
    float temperature = texture(temperatureTexture, UV).r;

    color = vec3(temperature, 0.0f, 0.0f);
}
