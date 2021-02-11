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
  std::string mName;
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
  GLuint mVertexArrayObject;

  glm::vec3 mAABBMin;
  glm::vec3 mAABBMax;
};
struct Material
{
  std::string mName;
  Renderer::Texture * mTextureDiffuse;
  Renderer::Texture * mTextureNormals;
  Renderer::Texture * mTextureSpecular;
  Renderer::Texture * mTextureAlbedo;
  Renderer::Texture * mTextureRoughness;
  Renderer::Texture * mTextureMetallic;
  Renderer::Texture * mTextureAO;
  glm::vec4 mColorDiffuse;
  glm::vec4 mColorSpecular;

  float mSpecularShininess;
};

bool LoadMesh( const char * _path );
void UnloadMesh();
std::string GetSupportedExtensions();

extern std::map<int, Node> mNodes;
extern std::map<int, Mesh> mMeshes;
extern std::map<int, Material> mMaterials;
extern glm::mat4x4 * mMatrices;
extern glm::vec3 mAABBMin;
extern glm::vec3 mAABBMax;
}