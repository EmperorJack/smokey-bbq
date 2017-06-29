#version 330 core

layout(origin_upper_left) in vec4 gl_FragCoord;

out vec3 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D densityTexture;

void main() {
    float ny = gl_FragCoord.x / screenWidth;
    float nx = gl_FragCoord.y / screenHeight;

    float density = texture(densityTexture, vec2(nx, ny)).r;
    float temperature = texture(densityTexture, vec2(nx, ny)).g;

    color = vec3(ny, nx, temperature) * (density * 5.0f);
    color = vec3(density, temperature, 0.0f);

    //color = vec3(nx, ny, 0.0f);
}
