#version 410 core

in vec3 in_pos;
in vec3 in_normal;
in vec3 in_tangent;
in vec3 in_binormal;
in vec2 in_texcoord;

out vec3 out_normal;
out vec3 out_tangent;
out vec3 out_binormal;
out vec2 out_texcoord;
out vec3 out_worldpos;

uniform mat4x4 mat_projection;
uniform mat4x4 mat_view;
uniform mat4x4 mat_world;

void main()
{
  vec4 o = vec4( in_pos.x, in_pos.y, in_pos.z, 1.0 );
  o = mat_world * o;
  out_worldpos = o.xyz;
  o = mat_view * o;
  o = mat_projection * o;
  gl_Position = o;
  
  out_normal = normalize( in_normal ) * mat3( mat_world );
  out_tangent = normalize( in_tangent ) * mat3( mat_world );
  out_binormal = normalize( in_binormal ) * mat3( mat_world );
  out_texcoord = in_texcoord;
}