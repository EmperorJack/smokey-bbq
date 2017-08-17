#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D divergenceTexture;
uniform sampler2D pressureTexture;

uniform int gridSize;
uniform float inverseSize;
uniform float fluidDensity;

bool clampBoundary(float i) {
    if (i < 0) {
        return true;
    }  else if (i >= gridSize) {
        return true;
    }

    return false;
}

float getGridPressure(float i, float j) {
    vec2 texcoord = vec2(i, j) * inverseSize;
     bool boundary = clampBoundary(i) || clampBoundary(j);
    return texture(pressureTexture, texcoord).x * (boundary ? 0.0f : 1.0f);
}

void main() {
    vec2 pos = gl_FragCoord.xy;
    float i = float(pos.x);
    float j = float(pos.y);

    float d = texture(divergenceTexture, pos * inverseSize).x;
    float p = getGridPressure(i + 2, j) +
              getGridPressure(i - 2, j) +
              getGridPressure(i, j + 2) +
              getGridPressure(i, j - 2);
    color = vec4((d + p) * 0.25f, 0.0f, 0.0f, 0.0f);
}
