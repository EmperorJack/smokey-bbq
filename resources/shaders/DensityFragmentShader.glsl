#version 330 core

in vec2 UV;

out vec3 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D densityTexture;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    float density = texture(densityTexture, UV).r;
    float temperature = texture(densityTexture, UV).g;

    float nx = gl_FragCoord.x / float(screenWidth);
    float ny = gl_FragCoord.y / float(screenHeight);

    color = hsv2rgb(vec3(nx, 1.0f, temperature)) * (density * 4.0f);
}
