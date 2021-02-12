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

uniform sampler2D tex_diffuse;
uniform sampler2D tex_normals;
uniform sampler2D tex_specular;

uniform bool has_tex_diffuse;
uniform bool has_tex_normals;
uniform bool has_tex_specular;
uniform bool has_tex_albedo;
uniform bool has_tex_roughness;
uniform bool has_tex_metallic;
uniform bool has_tex_ao;

out vec4 frag_color;

float calculate_specular( vec3 normal )
{
  vec3 V = normalize( camera_position - out_worldpos );
  vec3 L = normalize( light_direction );
  vec3 H = normalize( V + L );
  float rdotv = clamp( dot( normal, H ), 0.0, 1.0 );
  float total_specular = pow( rdotv, specular_shininess );

  return total_specular;
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

  float ndotl = dot( normal, -normalize( light_direction ) );

  vec3 color = ((has_tex_diffuse ? diffuse : color_diffuse.rgb) + color_specular.rgb * calculate_specular( normal ) * color_specular.a) * ndotl;

  frag_color = vec4( pow( color, vec3(1.0f) ), 1.0f );
}
