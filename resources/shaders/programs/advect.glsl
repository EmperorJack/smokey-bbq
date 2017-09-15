#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;
uniform sampler2D sourceTexture;

uniform int gridSize;
uniform float inverseSize;
uniform bool wrapBorders;
uniform float gridSpacing;
uniform float timeStep;
uniform float dissipation;

bool clampBoundary(inout float i) {
    if (i < 0) {
        i = 0;
        return true;
    }  else if (i >= gridSize) {
        i = gridSize - 1;
        return true;
    }

    return false;
}

vec3 getGridValue(sampler2D source, float i, float j) {
    bool boundary = !wrapBorders && (clampBoundary(i) || clampBoundary(j));
    vec2 texcoord = vec2(i, j) * inverseSize;
    return texture(source, texcoord).xyz * (boundary ? 0.0f : 1.0f);
}

vec3 getValue(sampler2D source, float x, float y) {
    float normX = x / float(gridSpacing);
    float normY = y / float(gridSpacing);

    vec3 v = (getGridValue(source, normX - 0.5f, normY) +
         getGridValue(source, normX + 0.5f, normY) +
         getGridValue(source, normX, normY - 0.5f) +
         getGridValue(source, normX, normY + 0.5f)) * 0.25f;

    return v;
}

vec2 traceParticle(float x, float y) {
    vec2 v = getValue(velocityTexture, x, y).xy;
    v = getValue(velocityTexture, x + (0.5f * timeStep * v.x), y + (0.5f * timeStep * v.y)).xy;
    return vec2(x, y) - (timeStep * v);
}

void main() {
    vec2 pos = gl_FragCoord.xy;

    vec2 tracePosition = traceParticle(pos.x * gridSpacing, pos.y * gridSpacing);
    vec3 newValue = getValue(sourceTexture, tracePosition.x, tracePosition.y);
    color = vec4(newValue * dissipation, 0.0f);
}
