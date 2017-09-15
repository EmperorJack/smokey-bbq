#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;

uniform int gridSize;
uniform float inverseSize;
uniform bool wrapBorders;
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
    bool boundary = !wrapBorders && (clampBoundary(i) || clampBoundary(j));
    vec2 texcoord = vec2(i, j) * inverseSize;
    return texture(source, texcoord).xy * (boundary ? 0.0f : 1.0f);
}

void main() {
    vec2 pos = gl_FragCoord.xy;
    int i = int(pos.x);
    int j = int(pos.y);

    float pdx = (getGridVelocity(velocityTexture, i + 1, j).y -
                 getGridVelocity(velocityTexture, i - 1, j).y) * 0.5f;
    float pdy = (getGridVelocity(velocityTexture, i, j + 1).x -
                 getGridVelocity(velocityTexture, i, j - 1).x) * 0.5f;

    color = vec4(pdx - pdy, 0.0f, 0.0f, 0.0f);
}
