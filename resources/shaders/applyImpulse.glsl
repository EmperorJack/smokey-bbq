#version 330 core

layout(location = 0) out vec4 color;

uniform float horizontalSpacing;
uniform float verticalSpacing;
uniform vec2 position;
uniform float radius;
uniform vec3 fill;
uniform bool outwardImpulse;

void main() {
    vec2 gridPosition = vec2(gl_FragCoord.x * horizontalSpacing, gl_FragCoord.y * verticalSpacing);
    float distance = distance(position, gridPosition);

    if (distance < radius) {
        float alpha = 1.0f - distance / radius;

        if (outwardImpulse) {
            color = vec4(length(fill) * normalize(gridPosition - position), 0.0f, alpha);
        } else {
            color = vec4(fill, alpha);
        }
    } else {
        color = vec4(0.0f);
    }
}
