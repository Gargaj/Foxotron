#version 410 core

in vec3 out_normal;
in vec3 out_tangent;
in vec3 out_binormal;
in vec2 out_texcoord;

out vec4 frag_color;

uniform sampler2D tex_diffuse;
uniform sampler2D tex_normals;
uniform sampler2D tex_specular;

void main(void)
{
  frag_color = texture( tex_diffuse, out_texcoord );
}