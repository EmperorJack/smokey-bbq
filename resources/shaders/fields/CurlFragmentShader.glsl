#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D curlTexture;

void main() {
    float curl = texture(curlTexture, UV).r / 10.0f;

    if (curl > 0.0) {
        // Clockwise
        color = vec4(curl, 0.0, 0.0, 1.0);
    } else {
        // Anti-clockwise
        color = vec4(0.0, abs(curl), 0.0, 1.0);
    }
}
