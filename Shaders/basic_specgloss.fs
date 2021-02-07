#version 410 core

in vec3 out_normal;
in vec3 out_tangent;
in vec3 out_binormal;
in vec2 out_texcoord;
in vec3 out_worldpos;

uniform vec3 camera_position;
uniform vec3 light_direction;

uniform sampler2D tex_diffuse;
uniform sampler2D tex_normals;
uniform sampler2D tex_specular;

out vec4 frag_color;

float calculate_specular( vec3 normal )
{
  float shininess = 8.0f;
  
  vec3 V = normalize( camera_position - out_worldpos );
  vec3 L = normalize( light_direction );
  vec3 H = normalize( V + L );
  float rdotv = clamp( dot( normal, H ), 0.0, 1.0 );
  float total_specular = pow( rdotv, shininess );

  return total_specular;
}

void main(void)
{
  vec4 diffuse = texture( tex_diffuse, out_texcoord );
  vec3 normalmap = texture( tex_normals, out_texcoord ).xyz;
  vec4 specular = texture( tex_specular, out_texcoord );

  vec3 normal = normalize( out_normal + normalmap.x * out_tangent + -normalmap.y * out_binormal );
  
  float ndotl = dot( normal, normalize( light_direction ) );

  vec4 color = diffuse * ndotl + specular * calculate_specular( normal );
  
  frag_color = pow( color, vec4(1.0 / 2.2) );
}