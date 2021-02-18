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

uniform vec4 color_ambient;
uniform vec4 color_diffuse;
uniform vec4 color_specular;
uniform float skysphere_rotation;

uniform vec3 camera_position;
uniform Light lights[3];

uniform sampler2D tex_skysphere;
uniform sampler2D tex_skyenv;
uniform sampler2D tex_albedo;
uniform sampler2D tex_diffuse;
uniform sampler2D tex_specular;
uniform sampler2D tex_normals;
uniform sampler2D tex_roughness;
uniform sampler2D tex_metallic;
uniform sampler2D tex_ao;
uniform sampler2D tex_ambient;

uniform bool has_tex_skysphere;
uniform bool has_tex_skyenv;
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

vec3 fresnel_schlick( vec3 H, vec3 V, vec3 F0 )
{
  float cosTheta = clamp( dot( H, V ), 0., 1. );
  return F0 + ( vec3( 1.0 ) - F0 ) * pow( 1. - cosTheta, 5.0 );
}

vec3 fresnel_schlick_roughness( vec3 H, vec3 V, vec3 F0, float roughness )
{
  float cosTheta = clamp( dot( H, V ), 0., 1. );
  return F0 + ( max( vec3( 1.0 - roughness ), F0 ) - F0 ) * pow( 1. - cosTheta, 5.0 );
}

float distribution_ggx( vec3 N, vec3 H, float roughness )
{
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max( 0., dot( N, H ) );
  float factor = NdotH * NdotH * ( a2 - 1. ) + 1.;

  return a2 / ( PI * factor * factor );
}

float geometry_schlick_ggx( vec3 N, vec3 V, float k )
{
  float NdotV = max( 0., dot( N, V ) );
  return NdotV / (NdotV * ( 1. - k ) + k );
}

float geometry_smith( vec3 N, vec3 V, vec3 L, float roughness )
{
  float r = roughness + 1.;
  float k = (r * r) / 8.;
  return geometry_schlick_ggx( N, V, k ) * geometry_schlick_ggx( N, L, k );
}

vec2 sphere_to_polar( vec3 normal )
{
  normal = normalize( normal );

  float ang = atan( normal.z, normal.x );

  return vec2( ang / PI / 2.0 + 0.5 + skysphere_rotation, 1. - acos( normal.y ) / PI );
}

vec3 sample_sky( vec3 normal )
{
  vec2 polar = sphere_to_polar( normal * vec3( 1., -1., 1. ) );
  return texture( tex_skysphere, polar ).rgb;
}

vec3 sample_irradiance_slow( vec3 normal, vec3 vertex_tangent )
{
  float delta = 0.10;

  // Construct the tangent space using the perturbed normal and the original vertex tangent.
  // Maybe there's a better way to do this?

  vec3 right = cross( normal, vertex_tangent );
  vec3 up = vertex_tangent;
  int numIrradianceSamples = 0;

  vec3 irradiance = vec3(0.);

  for ( float phi = 0.; phi < 2. * PI ; phi += delta )
  {
    for ( float theta = 0.; theta < 0.5 * PI; theta += delta )
    {
      vec3 tangent_space = vec3(
          sin( theta ) * cos( phi ),
          sin( theta ) * sin( phi ),
          cos( theta ) );
      vec3 world_space = tangent_space.x * right + tangent_space.y + up + tangent_space.z * normal;

      vec3 color = sample_sky( world_space ) * cos( theta ) * sin( theta );
      irradiance += color * cos( theta ) * sin( theta );
      numIrradianceSamples++;
    }
  }

  irradiance = PI * irradiance / float( numIrradianceSamples );
  return irradiance;
}

vec3 sample_irradiance_fast( vec3 normal, vec3 vertex_tangent )
{
  // Sample the irradiance map if it exists, otherwise fall back to blurred reflection map.
  if ( has_tex_skyenv )
  {
    // TODO: Is this y-axis flip really necessary?
    vec2 polar = sphere_to_polar( normal * vec3( 1., -1., 1. ) );
    // HACK: Sample a smaller mip here to avoid high frequency color variations on detailed normal
    //       mapped areas.
    return textureLod( tex_skyenv, polar, 2 ).rgb;
  }
  else
  {
    int level = 8;
    vec2 polar = sphere_to_polar( normal * vec3( 1., -1., 1. ) );
    // TODO how to properly normalize a single mipmap tap to irradiance?
    return textureLod( tex_skysphere, polar, level ).rgb;
  }
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

  vec3 N = normal;
  vec3 V = normalize( camera_position - out_worldpos );

  vec3 Lo = vec3(0.);
  vec3 F0 = vec3(0.04);
  F0 = mix( F0, baseColor, metallic );


  for ( int i = 0; i < lights.length(); i++ )
  {
    vec3 radiance = lights[ i ].color;

    vec3 L = -normalize( lights[ i ].direction );
    vec3 H = normalize( V + L );

    vec3 F = fresnel_schlick( H, V, F0 );
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float D = distribution_ggx( N, H, roughness );
    float G = geometry_smith( N, V, L, roughness );

    vec3 num = D * F * G;
    float denom = 4. * max( 0., dot( N, V ) ) * max( 0., dot( N, L ) );

    vec3 specular = kS * (num / max( 0.001, denom ));

    float NdotL = max( 0., dot( N, L ) );

    Lo += ( kD * ( baseColor / PI ) + specular ) * radiance * NdotL;
  }

  {
    vec3 irradiance = vec3(0.);

    bool bruteforce_irradiance = false;

    if (bruteforce_irradiance)
    {
      irradiance = sample_irradiance_slow( normal, out_tangent );
    }
    else
    {
      irradiance = sample_irradiance_fast( normal, out_tangent );
    }

    vec3 kS = fresnel_schlick_roughness( N, V, F0, roughness );
    vec3 kD = vec3(1.) - kS;
    vec3 diffuse = irradiance * baseColor;
    vec3 ambient = kD * diffuse;
    Lo += ambient * ao;
  }

  vec3 color = Lo;

  // tonemap and apply gamma correction
  color = color / ( vec3(1.) + color );
  frag_color = vec4( pow( color, vec3(1. / 2.2) ), 1.0f );
}
