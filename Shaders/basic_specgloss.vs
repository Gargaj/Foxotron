#version 410 core

in vec3 in_pos;
in vec2 in_texcoord;
out vec2 out_texcoord;

void main()
{
  gl_Position = vec4( in_pos.x, in_pos.y, in_pos.z, 1.0 );
  out_texcoord = in_texcoord;
};
