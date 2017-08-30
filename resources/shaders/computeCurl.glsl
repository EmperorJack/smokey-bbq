#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;

uniform int gridSize;
uniform float inverseSize;
uniform float gridSpacing;

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
    bool boundary = clampBoundary(i) || clampBoundary(j);
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

void main() {
    vec2 pos = gl_FragCoord.xy;
    float i = float(pos.x);
    float j = float(pos.y);

    float pdx = (getInterpolatedVelocity(velocityTexture, i + 1, j).x -
                 getInterpolatedVelocity(velocityTexture, i - 1, j).x) * 0.5f;
    float pdy = (getInterpolatedVelocity(velocityTexture, i, j + 1).y -
                 getInterpolatedVelocity(velocityTexture, i, j - 1).y) * 0.5f;

    color = vec4(pdx - pdy, 0.0f, 0.0f, 0.0f);
}
