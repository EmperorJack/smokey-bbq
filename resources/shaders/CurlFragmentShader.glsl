#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D curlTexture;

void main() {
    float curl = texture(curlTexture, UV).b;

    if (curl > 0.0f) {
        // Clockwise
        color = vec4(curl, 0.0f, 0.0f, 1.0);
    } else {
        // Anti-clockwise
        color = vec4(0.0f, abs(curl), 0.0f, 1.0);
    }
}
