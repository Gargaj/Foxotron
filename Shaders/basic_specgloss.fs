#version 410 core

struct Light
{
  vec3 direction;
  vec3 color;
};

struct ColorMap
{
  bool has_tex;
  sampler2D tex;
  vec4 color;
};

in vec3 out_normal;
in vec3 out_tangent;
in vec3 out_binormal;
in vec2 out_texcoord;
in vec3 out_worldpos;
in vec3 out_to_camera;


uniform float specular_shininess;

uniform float skysphere_rotation;
uniform float skysphere_mip_count;
uniform float exposure;

uniform vec3 camera_position;
uniform Light lights[3];

uniform vec4 global_ambient;

uniform bool has_tex_skysphere;
uniform bool has_tex_skyenv;

uniform sampler2D tex_skysphere;
uniform sampler2D tex_skyenv;

uniform ColorMap map_albedo;
uniform ColorMap map_diffuse;
uniform ColorMap map_specular;
uniform ColorMap map_normals;
uniform ColorMap map_roughness;
uniform ColorMap map_metallic;
uniform ColorMap map_ao;
uniform ColorMap map_ambient;
uniform ColorMap map_emissive;

out vec4 frag_color;

const float PI = 3.1415926536;

vec4 sample_colormap( ColorMap map, vec2 uv)
{
  return map.has_tex ? texture( map.tex, uv ) : map.color;
}

vec2 sphere_to_polar( vec3 normal )
{
  normal = normalize( normal );
  return vec2( ( atan( normal.z, normal.x ) + skysphere_rotation ) / PI / 2.0 + 0.5, acos( normal.y ) / PI );
}

vec3 sample_irradiance_fast( vec3 normal )
{
  // Sample the irradiance map if it exists, otherwise fall back to blurred reflection map.
  if ( has_tex_skyenv )
  {
    vec2 polar = sphere_to_polar( normal );
    // HACK: Sample a smaller mip here to avoid high frequency color variations on detailed normal
    //       mapped areas.
    float miplevel = 5.5; // tweaked for a 360x180 irradiance texture
    return textureLod( tex_skyenv, polar, miplevel ).rgb * exposure;
  }
  else
  {
    vec2 polar = sphere_to_polar( normal );
    return textureLod( tex_skysphere, polar, 0.80 * skysphere_mip_count ).rgb * exposure;
  }
}

float calculate_specular( vec3 normal, vec3 light_direction )
{
  vec3 V = normalize( out_to_camera );
  vec3 L = -normalize( light_direction );
  vec3 H = normalize( V + L );
  float rdotv = clamp( dot( normal, H ), 0.0, 1.0 );
  float total_specular = pow( rdotv, specular_shininess );

  return total_specular;
}

void main(void)
{
  vec3 ambient = sample_colormap( map_ambient, out_texcoord ).xyz;
  vec4 diffusemap_alpha = sample_colormap( map_diffuse, out_texcoord );
  vec3 diffusemap = diffusemap_alpha.xyz;
  float alpha = diffusemap_alpha.w;
  vec3 normalmap = normalize(texture( map_normals.tex, out_texcoord ).xyz * vec3(2.0) - vec3(1.0));
  vec4 specularmap = sample_colormap( map_specular, out_texcoord );

  vec3 normal = out_normal;
  if ( map_normals.has_tex )
  {
    // Mikkelsen's tangent space normal map decoding. See http://mikktspace.com/ for rationale.
    vec3 bi = cross( out_normal, out_tangent );
    vec3 nmap = normalmap.xyz;
    normal = nmap.x * out_tangent + nmap.y * bi + nmap.z * out_normal;
  }

  normal = normalize( normal );

  vec3 irradiance = sample_irradiance_fast( normal );
  ambient *= irradiance;

  vec3 color = ambient * global_ambient.rgb;
  for ( int i = 0; i < lights.length(); i++ )
  {
    float ndotl = clamp( dot( normal, -normalize( lights[ i ].direction ) ), 0.0, 1.0 );

    vec3 specular = specularmap.rgb * calculate_specular( normal, lights[ i ].direction ) * specularmap.a;
    color += (diffusemap + specular) * ndotl * lights[ i ].color;
  }

  color += sample_colormap( map_emissive, out_texcoord ).rgb;

  frag_color = vec4( pow( color * exposure, vec3(1. / 2.2) ), alpha );
}
