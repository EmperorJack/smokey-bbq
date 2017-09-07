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

vec2 getGridVelocity(sampler2D source, float i, float j) {
    bool boundary = !wrapBorders && (clampBoundary(i) || clampBoundary(j));
    vec2 texcoord = vec2(i, j) * inverseSize;
    return texture(source, texcoord).xy * (boundary ? 0.0f : 1.0f);
}

vec2 getInterpolatedVelocity(sampler2D source, float x, float y) {
//    int i = (int(x + gridSize)) - gridSize;
//    int j = (int(y + gridSize)) - gridSize;
    float i = x;
    float j = y;

    return (i+1-x) * (j+1-y) * getGridVelocity(source, i, j) +
           (x-i) * (j+1-y)   * getGridVelocity(source, i+1, j) +
           (i+1-x) * (y-j)   * getGridVelocity(source, i, j+1) +
           (x-i) * (y-j)     * getGridVelocity(source, i+1, j+1);
}

vec2 getVelocity(sampler2D source, float x, float y) {
    float normX = x / float(gridSpacing);
    float normY = y / float(gridSpacing);

    vec2 v = vec2(0.0f, 0.0f);

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedVelocity(source, normX - 0.5f, normY).x +
           getInterpolatedVelocity(source, normX + 0.5f, normY).x) * 0.5f;
    v.y = (getInterpolatedVelocity(source, normX, normY - 0.5f).y +
           getInterpolatedVelocity(source, normX, normY + 0.5f).y) * 0.5f;

    return v;
}

vec2 traceParticle(float x, float y) {
    vec2 v = getVelocity(velocityTexture, x, y);
    v = getVelocity(velocityTexture, x + (0.5f * timeStep * v.x), y + (0.5f * timeStep * v.y));
    return vec2(x, y) - (timeStep * v);
}

void main() {
    vec2 pos = gl_FragCoord.xy;

    vec2 tracePosition = traceParticle(pos.x * gridSpacing, pos.y * gridSpacing);
    vec2 newValue = getVelocity(sourceTexture, tracePosition.x, tracePosition.y);
    color = vec4(newValue.xy * dissipation, 0.0f, 0.0f);
}
