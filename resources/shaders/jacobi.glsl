#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D divergenceTexture;
uniform sampler2D pressureTexture;

uniform int gridSize;
uniform float inverseSize;

float clampIndex(float i) {
    if (i < 0) {
        return 0.0f;
    }  else if (i >= gridSize) {
        return float(gridSize - 1);
    }

    return i;
}

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

float getGridPressure(float i, float j) {
    bool boundary = clampBoundary(i) || clampBoundary(j);
    vec2 texcoord = vec2(i, j) * inverseSize;
    return texture(pressureTexture, texcoord).x * (boundary ? 0.0f : 1.0f);
}

void main() {
    vec2 pos = gl_FragCoord.xy;
    float i = float(pos.x);
    float j = float(pos.y);

    float d = texture(divergenceTexture, pos * inverseSize).x;
    float p = getGridPressure(clampIndex(i + 2), j) +
              getGridPressure(clampIndex(i - 2), j) +
              getGridPressure(i, clampIndex(j + 2)) +
              getGridPressure(i, clampIndex(j - 2));
    color = vec4((d + p) * 0.25f, 0.0f, 0.0f, 0.0f);
}
