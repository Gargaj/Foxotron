#version 410 core

in vec3 out_normal;
in vec3 out_tangent;
in vec3 out_binormal;
in vec2 out_texcoord;
in vec3 out_worldpos;

uniform float specular_shininess;
uniform vec4 color_diffuse;
uniform vec4 color_specular;

uniform vec3 camera_position;
uniform vec3 light_direction;

uniform sampler2D tex_albedo;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_specular;
uniform sampler2D tex_normals;
uniform sampler2D tex_roughness;
uniform sampler2D tex_metallic;
uniform sampler2D tex_ao;
uniform sampler2D tex_ambient;

uniform bool has_tex_diffuse;
uniform bool has_tex_normals;
uniform bool has_tex_specular;
uniform bool has_tex_albedo;
uniform bool has_tex_roughness;
uniform bool has_tex_metallic;
uniform bool has_tex_ao;
uniform bool has_tex_ambient;

out vec4 frag_color;

const float PI = 3.1415926536;

float calculate_specular( vec3 normal )
{
  vec3 V = normalize( camera_position - out_worldpos );
  vec3 L = normalize( light_direction );
  vec3 H = normalize( V + L );
  float rdotv = clamp( dot( normal, H ), 0.0, 1.0 );
  float total_specular = pow( rdotv, specular_shininess );

  return total_specular;
}

vec3 fresnelSchlick( vec3 H, vec3 V, vec3 F0 )
{
  float cosTheta = clamp( dot( H, V ), 0., 1. );
  return F0 + ( vec3( 1.0 ) - F0 ) * pow( 1. - cosTheta, 5.0 );
}

float distributionGGX( vec3 N, vec3 H, float roughness )
{
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max( 0., dot( N, H ) );
  float factor = NdotH * NdotH * ( a2 - 1. ) + 1.;

  return a2 / ( PI * factor * factor );
}

float geometrySchlickGGX( vec3 N, vec3 V, float k )
{
  float NdotV = max( 0., dot( N, V ) );
  return NdotV / (NdotV * ( 1. - k ) + k );
}

float geometrySmith( vec3 N, vec3 V, vec3 L, float roughness )
{
  float r = roughness + 1.;
  float k = (r * r) / 8.;
  return geometrySchlickGGX( N, V, k ) * geometrySchlickGGX( N, L, k );
}

void main(void)
{
  vec3 baseColor = color_diffuse.rgb;
  float roughness = 1.0;
  float metallic = 0.0;
  float ao = 0.0;

  if ( has_tex_albedo )
    baseColor = texture( tex_albedo, out_texcoord ).xyz;
  else if ( has_tex_diffuse )
    baseColor = texture( tex_diffuse, out_texcoord ).xyz;

  if ( has_tex_roughness )
    roughness = texture( tex_roughness, out_texcoord ).x;

  if ( has_tex_metallic )
    metallic = texture( tex_metallic, out_texcoord ).x;

  if ( has_tex_ao )
    ao = texture( tex_ao, out_texcoord ).x;
  else if ( has_tex_ambient )
    ao = texture( tex_ambient, out_texcoord ).x;

  vec3 normalmap = normalize(texture( tex_normals, out_texcoord ).xyz * vec3(2.0) - vec3(1.0));

  vec3 normal = out_normal;

  if (has_tex_normals)
  {
    // Mikkelsen's tangent space normal map decoding. See http://mikktspace.com/ for rationale.
    vec3 bi = cross( out_normal, out_tangent );
    vec3 nmap = normalmap.xyz;
    normal = nmap.x * out_tangent + nmap.y * bi + nmap.z * out_normal;
  }

  normal = normalize( normal );

  vec3 L = -normalize( light_direction );
  vec3 N = normal;
  vec3 V = normalize( camera_position - out_worldpos );
  vec3 H = normalize( V + L );

  vec3 Lo = vec3(0.);

  vec3 F0 = vec3(0.04);
  F0 = mix( F0, baseColor, metallic );
  vec3 F = fresnelSchlick( H, V, F0 );
  vec3 radiance = vec3(1.0);
  vec3 ambient = vec3(0.01, 0.01, 0.02);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  float D = distributionGGX( N, H, roughness );
  float G = geometrySmith( N, V, L, roughness );

  vec3 num = D * F * G;
  float denom = 4. * max( 0., dot( N, V ) ) * max( 0., dot( N, L ) );

  vec3 specular = kS * (num / max( 0.001, denom ));

  float NdotL = max( 0., dot( N, L ) );

  Lo += ( kD * ( baseColor / PI ) + specular ) * radiance * NdotL;
  Lo += ambient * ao;

  vec3 color = Lo;

  // tonemap and apply gamma correction
  color = color / ( vec3(1.) + color );
  frag_color = vec4( pow( color, vec3(1. / 2.2) ), 1.0f );
}
