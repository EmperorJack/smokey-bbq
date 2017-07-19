#version 330 core

in vec2 UV;

out vec3 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D curlTexture;

void main() {
    float curl = texture(curlTexture, UV).r;

    if (curl > 0.0f) {
        // Clockwise
        color = vec3(curl, 0.0f, 0.0f);
    } else {
        // Anti-clockwise
        color = vec3(0.0f, abs(curl), 0.0f);
    }
}
