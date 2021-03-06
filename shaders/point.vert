#version 330

const float POINT_RADIUS = 16.0;

uniform mat4 view;
uniform mat4 proj;

in vec3 position;
in vec3 color;

out vec3 vertex_color;

void main() {
  vertex_color = color;

  vec4 pos = proj * view * vec4(position, 1.0);

  gl_PointSize = POINT_RADIUS / pos.w;
  gl_Position = pos;
}
