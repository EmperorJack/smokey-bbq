#version 330 core

in vec2 UV;

out vec4 color;

uniform int screenWidth;
uniform int screenHeight;

uniform sampler2D temperatureTexture;

const vec4 HIGH = vec4(1.0, 0.0, 0.07, 1.0);
const vec4 MIDDLE = vec4(0.07, 1.0, 0.07, 1.0);
const vec4 LOW   = vec4(0.07, 0.0, 1.0, 1.0);

float remap(float minVal, float maxVal, float curVal) {
    return (curVal - minVal) / (maxVal - minVal);
}

void main() {
    float temperature = texture(temperatureTexture, UV).r;

    temperature = clamp(temperature, 0.0, 1.0);
    if(temperature > 0.5) {
        color = mix(MIDDLE, HIGH, remap(0.5, 1.0, temperature));
    }
    else {
        color = mix(LOW, MIDDLE, remap(0.0, 0.5, temperature));
    }
}
