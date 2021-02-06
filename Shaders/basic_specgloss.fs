#version 410 core

in vec3 out_normal;
in vec3 out_tangent;
in vec3 out_binormal;
in vec2 out_texcoord1;
in vec2 out_texcoord2;

out vec4 frag_color;

void main(void)
{
  frag_color = vec4( out_normal.x, out_normal.y, out_normal.z, 1 );
}