#version 330

uniform float time;
uniform float deltatime;

in vec3 vertex_color;

out vec4 out_color;

void main() {
  // out_color = vec4(gl_PointCoord.x,0.0,gl_PointCoord.y,1.0);
  if(length(gl_PointCoord - vec2(0.5)) > 0.5) {
    discard;
  }
  // vec3 color = vec3(0.7,0.8,0.9);
  out_color = vec4(vertex_color, 1.0);
}
