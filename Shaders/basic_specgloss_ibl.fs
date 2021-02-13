#version 410 core

struct Light
{
  vec3 direction;
  vec3 color;
};

in vec3 out_normal;
in vec3 out_tangent;
in vec3 out_binormal;
in vec2 out_texcoord;
in vec3 out_worldpos;

uniform float specular_shininess;
uniform vec4 color_ambient;
uniform vec4 color_diffuse;
uniform vec4 color_specular;

uniform vec3 camera_position;
uniform Light lights[3];

uniform sampler2D tex_skysphere;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_normals;
uniform sampler2D tex_specular;

uniform bool has_tex_skysphere;
uniform bool has_tex_diffuse;
uniform bool has_tex_normals;
uniform bool has_tex_specular;
uniform bool has_tex_albedo;
uniform bool has_tex_roughness;
uniform bool has_tex_metallic;
uniform bool has_tex_ao;

float texture_lod = 0.0;
uniform float skysphere_rotation;

out vec4 frag_color;

const float PI = 3.1415926536;

vec2 sphere_to_polar( vec3 normal )
{
  normal = normalize( normal );
  return vec2( atan(normal.z, normal.x) / PI / 2.0 + 0.5 + skysphere_rotation, 1. - acos(normal.y) / PI );
}

void main(void)
{
  vec3 diffuse = texture( tex_diffuse, out_texcoord ).xyz;
  vec3 normalmap = normalize(texture( tex_normals, out_texcoord ).xyz * vec3(2.0) - vec3(1.0));
  vec3 specular = has_tex_specular ? texture( tex_specular, out_texcoord ).xyz * color_specular.rgb : vec3(0.0f);

  vec3 normal = out_normal;

  if (has_tex_normals)
  {
    // Mikkelsen's tangent space normal map decoding. See http://mikktspace.com/ for rationale.
    vec3 bi = cross( out_normal, out_tangent );
    vec3 nmap = normalmap.xyz;
    normal = nmap.x * out_tangent + nmap.y * bi + nmap.z * out_normal;
  }

  normal = normalize( normal );

  vec3 incident = normalize( out_worldpos - camera_position );

  vec3 reflection_angle = normalize( incident - 2 * normal * dot(incident, normal) );

  vec3 sky_color = textureLod( tex_skysphere, sphere_to_polar( normal ), texture_lod ).rgb;

  frag_color = vec4( sky_color, 1.0f );
}
