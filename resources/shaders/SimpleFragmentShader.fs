#version 330 core

in vec2 UV;

out vec3 color;

uniform sampler2D uTexture;

uniform bool useFillColor = false;
uniform vec4 fillColor = vec4(0.0, 0.0, 0.0, 0.0);

uniform sampler2D myTextureSampler;

void main() {
  if (useFillColor) {
    color = fillColor.rgb;
  } else {
    float density = texture(myTextureSampler, UV).r;
    color = vec3(0.0f, density, density);
  }
}
