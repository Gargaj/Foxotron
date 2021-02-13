#version 410 core

in vec3 in_pos;

out vec3 out_worldpos;

uniform mat4x4 mat_projection;
uniform mat4x4 mat_view;

void main()
{
  vec4 o = vec4( in_pos.x, in_pos.y, in_pos.z, 1.0 );
  out_worldpos = o.xyz;
  o = mat_view * o;
  o = mat_projection * o;
  gl_Position = o;
}