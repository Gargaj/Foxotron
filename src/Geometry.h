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
  unsigned int mID;
  std::vector<unsigned int> mMeshes;
  unsigned int mParentID;
  glm::mat4x4 mTransformation;
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
  Renderer::Texture * mTextureDiffuse;
  Renderer::Texture * mTextureNormals;
  Renderer::Texture * mTextureSpecular;
  Renderer::Texture * mTextureAlbedo;
  Renderer::Texture * mTextureRoughness;
  Renderer::Texture * mTextureMetallic;
};

bool LoadMesh( const char * _path );
void UnloadMesh();
std::string GetSupportedExtensions();

extern std::map<int, Node> mNodes;
extern std::map<int, Mesh> mMeshes;
extern std::map<int, Material> mMaterials;
extern glm::mat4x4 * mMatrices;
}