#version 330 core

//layout(origin_upper_left) in vec4 gl_FragCoord;
layout(location = 0) out vec3 color;

uniform sampler2D velocityTexture;
uniform sampler2D sourceTexture;

uniform int gridSize;
uniform float gridSpacing;
uniform float timeStep;
uniform float dissipation;

vec2 getGridVelocity(sampler2D source, float i, float j) {
    vec2 texcoord = vec2(i, j); // / float(gridSize);
    return texture(source, texcoord).xy;
}

float getInterpolatedVelocity(sampler2D source, float x, float y, bool xAxis) {
    int i = int(x); //(int(x + gridSize)) - gridSize;
    int j = int(y); //(int(y + gridSize)) - gridSize;

    return (i+1-x) * (j+1-y) * (xAxis ? getGridVelocity(source, i, j).x : getGridVelocity(source, i, j).y) +
           (x-i) * (j+1-y)   * (xAxis ? getGridVelocity(source, i+1, j).x : getGridVelocity(source, i+1, j).y) +
           (i+1-x) * (y-j)   * (xAxis ? getGridVelocity(source, i, j+1).x : getGridVelocity(source, i, j+1).y) +
           (x-i) * (y-j)     * (xAxis ? getGridVelocity(source, i+1, j+1).x : getGridVelocity(source, i+1, j+1).y);
}

vec2 getVelocity(sampler2D source, float x, float y) {
    float normX = x / gridSpacing;
    float normY = y / gridSpacing;

    vec2 v = vec2(0.0f, 0.0f);

    // Evaluating staggered grid velocities using central differences
    v.x = (getInterpolatedVelocity(source, normX - 0.5f, normY, true) +
           getInterpolatedVelocity(source, normX + 0.5f, normY, true)) * 0.5f;
    v.y = (getInterpolatedVelocity(source, normX, normY - 0.5f, false) +
           getInterpolatedVelocity(source, normX, normY + 0.5f, false)) * 0.5f;

//    v.x = getInterpolatedVelocity(normX, normY-0.5f, true);
//    v.y = getInterpolatedVelocity(normX-0.5f, normY, false);

    return v;
}

vec2 traceParticle(float x, float y) {
    vec2 v = getVelocity(velocityTexture, x, y);
    v = getVelocity(velocityTexture, x + (0.5f * timeStep * v.x), y + (0.5f * timeStep * v.y));
    return vec2(x, y) - (timeStep * v);
}

void main() {
//    vec2 texcoord = gl_FragCoord.xy / gridSize;
//    vec2 coords = gl_FragCoord.xy;

    // Comes in as value 0-gridsize
    vec2 pos = vec2(gl_FragCoord);// + vec2(0.5, 0.5);

    //int i = int(pos.x); // * float(gridSize)
    //int j = int(pos.y); // * float(gridSize)

//    if ((gridSize / 2 - 10) < pos.x && pos.x < (gridSize / 2 + 10) &&
//        (gridSize / 2 - 10) < pos.y && pos.y < (gridSize / 2 + 10)) {
//        color = vec3(75.0f, 25.0f, 0.0f);
//        return;
//    } else {
//        color = vec3(0.0f, 1.0f, 0.0f);
//        return;
//    }

//    vec2 tracePosition = traceParticle(pos.x * gridSpacing, pos.y * gridSpacing);
//    vec2 newValue = getVelocity(sourceTexture, tracePosition.x, tracePosition.y);
//    color = vec3(newValue.xy * dissipation, 0.0f);

    color = vec3(1.0f, 0.0f, 0.0f);

//    color = vec3(getVelocity(i * gridSpacing, j * gridSpacing), 0.0f);

    //vec2 pos = coords - timeStep * (1 / float(gridSize)) * texture(velocityTexture, coords).xy;
    //color = vec3(getVelocity(sourceTexture, pos.x, pos.y), 0.0f);

//    vec2 fragCoord = gl_FragCoord.xy;
//    vec2 u = texture(velocityTexture, rdx * fragCoord).xy;
//    vec2 tracePosition = rdx * (fragCoord - timeStep * u);
//    color = vec3(getVelocity(sourceTexture, tracePosition.x, tracePosition.y), 0.0f);
}
