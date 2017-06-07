#version 330 core
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;

out vec2 UV;

uniform mat4 MVP;

void main() {
  //gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
  gl_Position = vec4(vertexPosition_modelspace, 1);

  UV = vec2(vertexPosition_modelspace.x, -vertexPosition_modelspace.y);
  UV = UV.xy * 0.5f + 0.5f;
}