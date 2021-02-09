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
  vec3 specular = texture( tex_specular, out_texcoord ).xyz * color_specular.rgb;

  vec3 normal = normalize( out_normal + normalmap.x * out_tangent + -normalmap.y * out_binormal );
  
  float ndotl = dot( normal, normalize( light_direction ) );

  vec3 color = diffuse * ndotl + color_specular.rgb * calculate_specular( normal ) * color_specular.a;
  
  frag_color = vec4( pow( color, vec3(1.0f / 2.2f) ), 1.0f );
}