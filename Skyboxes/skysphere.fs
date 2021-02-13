#version 410 core

in vec3 out_worldpos;

uniform float texture_lod;
uniform sampler2D tex_skysphere;
uniform float skysphere_rotation;
uniform float skysphere_blur;
uniform float skysphere_opacity;

out vec4 frag_color;

const float PI = 3.1415926536;

vec2 sphere_to_polar( vec3 normal )
{
  normal = normalize( normal );
  return vec2( atan(normal.z, normal.x) / PI / 2.0 + 0.5 + skysphere_rotation, 1. - acos(normal.y) / PI );
}

void main(void)
{
  vec3 sky_color = textureLod( tex_skysphere, sphere_to_polar( normalize( out_worldpos * vec3( 1.0,-1.0,-1.0) ) ), skysphere_blur ).rgb;

  vec3 gray = vec3(0.5);
  frag_color = vec4( mix(gray,sky_color,skysphere_opacity), 1.0f );
}
