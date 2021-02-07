#include <map>
#include <vector>
#include <string>

#include "Renderer.h"

#define GLEW_NO_GLU
#include "GL/glew.h"
#ifdef _WIN32
#include <GL/wGLew.h>
#endif

namespace Geometry
{
struct Node
{
  unsigned int nID;
  std::vector<unsigned int> mMeshes;
  unsigned int nParentID;
  glm::mat4x4 matTransformation;
};
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
std::string GetSupportedExtensions();

extern std::map<int, Node> mNodes;
extern std::map<int, Mesh> mMeshes;
extern std::map<int, Material> mMaterials;
}