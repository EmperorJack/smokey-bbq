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

vec3 getGridValue(sampler2D sampler, float i, float j) {
    bool boundary = !wrapBorders && (clampBoundary(i) || clampBoundary(j));
    vec2 texcoord = vec2(i, j) * inverseSize;
    return texture(sampler, texcoord).xyz * (boundary ? 0.0f : 1.0f);
}

vec2 getInterpolatedVelocity(float x, float y) {
//    int i = (int(x + gridSize)) - gridSize;
//    int j = (int(y + gridSize)) - gridSize;
    float i = x;
    float j = y;

    return (i+1-x) * (j+1-y) * getGridValue(velocityTexture, i, j).xy +
           (x-i) * (j+1-y)   * getGridValue(velocityTexture, i+1, j).xy +
           (i+1-x) * (y-j)   * getGridValue(velocityTexture, i, j+1).xy +
           (x-i) * (y-j)     * getGridValue(velocityTexture, i+1, j+1).xy;
}

vec2 getVelocity(float x, float y) {
    float normX = x / float(gridSpacing);
    float normY = y / float(gridSpacing);

    vec2 v = vec2(0.0f, 0.0f);

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedVelocity(normX - 0.5f, normY).x +
           getInterpolatedVelocity(normX + 0.5f, normY).x) * 0.5f;
    v.y = (getInterpolatedVelocity(normX, normY - 0.5f).y +
           getInterpolatedVelocity(normX, normY + 0.5f).y) * 0.5f;

    return v;
}

vec2 traceParticle(float x, float y) {
    vec2 v = getVelocity(x, y);
    v = getVelocity(x + (0.5f * timeStep * v.x), y + (0.5f * timeStep * v.y));
    return vec2(x, y) - (timeStep * v);
}

void main() {
    vec2 pos = gl_FragCoord.xy;

    vec2 tracePosition = traceParticle(pos.x * gridSpacing, pos.y * gridSpacing);
    vec3 newValue = getGridValue(sourceTexture, tracePosition.x / float(gridSpacing), tracePosition.y / float(gridSpacing));
    color = vec4(newValue * dissipation, 0.0f);
}
