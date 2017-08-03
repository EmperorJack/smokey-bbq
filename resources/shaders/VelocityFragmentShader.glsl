#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D velocityTexture;

void main() {
    float xVel = texture(velocityTexture, UV).r;
    float yVel = texture(velocityTexture, UV).g;

    color = vec4(abs(xVel), abs(yVel), 0.0, 1.0);
}
