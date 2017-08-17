#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;

uniform int gridSize;
uniform float inverseSize;
uniform float gridSpacing;
uniform float gradientScale;

bool clampBoundary(float i) {
    if (i < 0) {
        return true;
    }  else if (i >= gridSize) {
        return true;
    }

    return false;
}

vec2 getGridVelocity(sampler2D source, float i, float j) {
    vec2 texcoord = vec2(i, j) * inverseSize;
    bool boundary = clampBoundary(i) || clampBoundary(j);
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

void main() {
    vec2 pos = gl_FragCoord.xy;
    float i = float(pos.x);
    float j = float(pos.y);

    float d = getVelocity(velocityTexture, (i + 1) * gridSpacing, j * gridSpacing).x -
              getVelocity(velocityTexture, (i - 1) * gridSpacing, j * gridSpacing).x +
              getVelocity(velocityTexture, i * gridSpacing, (j + 1) * gridSpacing).y -
              getVelocity(velocityTexture, i * gridSpacing, (j - 1) * gridSpacing).y;

    color = vec4(gradientScale * d, 0.0f, 0.0f, 0.0f);
}
