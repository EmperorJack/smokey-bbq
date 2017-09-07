#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D curlTexture;

uniform int gridSize;
uniform float inverseSize;
uniform bool wrapBorders;
uniform float timeStep;
uniform float vorticityConfinementForce;

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

float getGridCurl(float i, float j) {
    bool boundary = !wrapBorders && (clampBoundary(i) || clampBoundary(j));
    vec2 texcoord = vec2(i, j) * inverseSize;
    return texture(curlTexture, texcoord).x * (boundary ? 0.0f : 1.0f);
}

void main() {
    vec2 pos = gl_FragCoord.xy;
    float i = float(pos.x);
    float j = float(pos.y);

    float curl = getGridCurl(i, j);
    float curlLeft = getGridCurl(i - 1, j);
    float curlRight = getGridCurl(i + 1, j);
    float curlBottom = getGridCurl(i, j - 1);
    float curlTop = getGridCurl(i, j + 1);

    vec3 magnitude = vec3(abs(curlRight) - abs(curlLeft), abs(curlTop) - abs(curlBottom), 0.0f);

    float length = length(magnitude);
    magnitude = length > 0.0f ? (magnitude / length) : vec3(0.0f);

    vec3 curlVector = vec3(0.0f, 0.0f, curl);
    vec3 force = timeStep * vorticityConfinementForce * cross(magnitude, curlVector);

    color = vec4(force.x, force.y, 0.0f, 0.0f);
}
