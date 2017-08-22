#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D pressureTexture;
uniform sampler2D velocityTexture;

uniform int gridSize;
uniform float inverseSize;
uniform float gradientScale;

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

    float xChange = getGridPressure(clampIndex(i + 1), j) - getGridPressure(clampIndex(i - 1), j);
    float yChange = getGridPressure(i, clampIndex(j + 1)) - getGridPressure(i, clampIndex(j - 1));

    vec2 vel = texture(velocityTexture, pos * inverseSize).xy;
    vec2 newVel = vel + vec2(gradientScale * xChange, gradientScale * yChange);

    color = vec4(newVel, 0.0f, 0.0f);
}
