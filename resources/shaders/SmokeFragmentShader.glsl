#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D textureA;
uniform sampler2D textureB;

void main() {
    float density = texture(textureA, UV).r;
    float temperature = texture(textureB, UV).r;

    float nx = gl_FragCoord.x / float(screenWidth);
    float ny = gl_FragCoord.y / float(screenHeight);

    //color = vec3(ny, nx, temperature) * (density * 5.0f);
    color = vec4(temperature, density, density, 1.0);
}
