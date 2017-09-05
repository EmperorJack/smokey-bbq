#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D temperatureTexture;
uniform sampler2D densityTexture;

uniform float inverseSize;
uniform float fallForce;
uniform float riseForce;
uniform float atmosphereTemperature;
uniform float gravity;

void main() {
    vec2 pos = gl_FragCoord.xy;

    float temperature = texture(temperatureTexture, pos * inverseSize).r;
    float density = texture(densityTexture, pos * inverseSize).r;

    vec2 force = (fallForce * density - riseForce * (temperature - atmosphereTemperature)) * vec2(0.0f, gravity / abs(gravity));

    color = vec4(force, 0.0f, 0.0f);
}
