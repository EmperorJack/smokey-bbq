#version 330 core

in vec2 UV;

out vec3 color;

uniform sampler2D densityTexture;

void main() {
    float density = texture(densityTexture, UV).r;
    float temperature = texture(densityTexture, UV).g;
    color = vec3(temperature, density, density);
}
