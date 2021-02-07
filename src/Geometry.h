#include <map>
#include "Renderer.h"

#define GLEW_NO_GLU
#include "GL/glew.h"
#ifdef _WIN32
#include <GL/wGLew.h>
#endif

namespace Geometry
{
struct Mesh
{
  int mVertexCount;
  GLuint mVertexBufferObject;
  int mTriangleCount;
  GLuint mIndexBufferObject;
  int mMaterialIndex;
};
struct Material
{
  Renderer::Texture * textureDiffuse;
  Renderer::Texture * textureNormals;
  Renderer::Texture * textureSpecular;
  Renderer::Texture * textureAlbedo;
  Renderer::Texture * textureRoughness;
  Renderer::Texture * textureMetallic;
};

bool LoadMesh( const char * _path );
void UnloadMesh();

extern std::map<int, Mesh> mMeshes;
extern std::map<int, Material> mMaterials;
}