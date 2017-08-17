#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;
uniform sampler2D sourceTexture;

uniform int gridSize;
uniform float inverseSize;
uniform float gridSpacing;
uniform float timeStep;
uniform float dissipation;

void main() {
//    vec2 pos = gl_FragCoord.xy;
//
//    vec2 tracePosition = traceParticle(pos.x * gridSpacing, pos.y * gridSpacing);
//    vec2 newValue = getVelocity(sourceTexture, tracePosition.x, tracePosition.y);
//    color = vec4(newValue.xy * dissipation, 0.0f, 0.0f);
    color = vec4(0.0f);
}
