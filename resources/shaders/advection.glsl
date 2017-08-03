#version 330 core

//layout(origin_upper_left) in vec4 gl_FragCoord;
layout(location = 0) out vec4 color;

uniform sampler2D velocityTexture;
uniform sampler2D sourceTexture;

uniform int gridSize;
uniform float gridSpacing;
uniform float timeStep;
uniform float dissipation;

bool clampBoundary(float i) {
    if (i < 0) {
        return true;
    }  else if (i >= gridSize) {
        return true;
    }

    return false;
}

vec2 getGridVelocity(sampler2D source, float i, float j) {
//    if (i < 0) i = 0;
//    if (i >= gridSize) i = gridSize - 1;
//    if (j < 0) j = 0;
//    if (j >= gridSize) j = gridSize - 1;

    vec2 texcoord = vec2(i, j) / float(gridSize);
    bool boundary = clampBoundary(i) || clampBoundary(j);
    return texture(source, texcoord).xy * (boundary ? 0.0f : 1.0f);
}

vec2 getInterpolatedVelocity(sampler2D source, float x, float y) {
    int i = (int(x + gridSize)) - gridSize;
    int j = (int(y + gridSize)) - gridSize;
//    float i = x;
//    float j = y;

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

//    color = vec3(pos.x, pos.y, 0.0f);
//    return;

    int i = int(pos.x);
    int j = int(pos.y);

//    if ((gridSize / 2 - 4) < pos.x && pos.x < (gridSize / 2 + 4) &&
//        (gridSize / 2 - 4) < pos.y && pos.y < (gridSize / 2 + 4)) {
//        color = vec4(100.0f, 0.0f, 0.0f, 0.0f);
//        return;
//    }

    vec2 tracePosition = traceParticle(pos.x * gridSpacing, pos.y * gridSpacing);
//    vec2 tracePosition = traceParticle(i * gridSpacing, j * gridSpacing);
    vec2 newValue = getVelocity(sourceTexture, tracePosition.x, tracePosition.y);
    color = vec4(newValue.xy * dissipation, 0.0f, 0.0f);
//    color = vec4(pos.x * gridSpacing, tracePosition.x, 0.0f, 0.0f);
//    color = vec4(1.0f, 0.0f, 0.0f, 0.0f);

//    float inverseSize = 1.0f / float(gridSize);
//    vec2 u = texture(velocityTexture, pos * inverseSize).xy;
//    vec2 coord = inverseSize * (pos - timeStep * u);
//    vec2 uNew = texture(sourceTexture, coord).xy;// * dissipation;
//    color = vec4(uNew.x, uNew.y, 0.0f, 0.0f);
//    color = vec3(10.0f, 0.0f, 0.0f);
//    color = vec3(u.y, u.x, 0);

//    float inverseSize = 1.0f / float(gridSize);
//    vec2 u = texture(velocityTexture, inverseSize * pos).xy;
//    vec2 coord = inverseSize * (pos - timeStep * u);
//    color = vec3(texture(sourceTexture, coord).xy, 0.0f) * dissipation;

//    color = vec3(1.0f, 0.0f, 0.0f);

//    color = vec3(getVelocity(i * gridSpacing, j * gridSpacing), 0.0f);

//    vec2 pos = coords - timeStep * (1 / float(gridSize)) * texture(velocityTexture, coords).xy;
//    color = vec3(getVelocity(sourceTexture, pos.x, pos.y), 0.0f);

//    vec2 fragCoord = gl_FragCoord.xy;
//    vec2 u = texture(velocityTexture, rdx * fragCoord).xy;
//    vec2 tracePosition = rdx * (fragCoord - timeStep * u);
//    color = vec3(getVelocity(sourceTexture, tracePosition.x, tracePosition.y), 0.0f);
}
