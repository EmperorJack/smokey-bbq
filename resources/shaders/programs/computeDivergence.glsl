#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;

uniform int gridSize;
uniform float inverseSize;
uniform bool wrapBorders;
uniform float gridSpacing;
uniform float gradientScale;

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

vec2 getVelocity(sampler2D source, float x, float y) {
    float normX = x / float(gridSpacing);
    float normY = y / float(gridSpacing);

    vec2 v = (getGridVelocity(source, normX - 0.5f, normY) +
         getGridVelocity(source, normX + 0.5f, normY) +
         getGridVelocity(source, normX, normY - 0.5f) +
         getGridVelocity(source, normX, normY + 0.5f)) * 0.25f;

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
