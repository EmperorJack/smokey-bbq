#version 330 core

in vec2 UV;

out vec3 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D densityTexture;

void main() {
    float density = texture(densityTexture, UV).r;
    float temperature = texture(densityTexture, UV).g;

    float nx = gl_FragCoord.x / float(screenWidth);
    float ny = gl_FragCoord.y / float(screenHeight);

//    color = vec3(ny, nx, temperature) * (density * 5.0f);
//    color = vec3(temperature, density, 0.0f);

    color = vec3(nx, ny, 0.0f);
}
