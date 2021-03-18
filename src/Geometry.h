#include <map>
#include <vector>
#include <string>

#include "Renderer.h"

#define GLEW_NO_GLU
#include "GL/glew.h"
#ifdef _WIN32
#include <GL/wGLew.h>
#endif

class Geometry
{
public:
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

    bool mTransparent;
  };
  struct ColorMap
  {
    ColorMap() : mValid( false ), mTexture( nullptr ), mColor( 0.0f ) {}
    bool mValid;
    Renderer::Texture * mTexture;
    glm::vec4 mColor;
  };
  struct Material
  {
    std::string mName;
    ColorMap mColorMapDiffuse;
    ColorMap mColorMapNormals;
    ColorMap mColorMapSpecular;
    ColorMap mColorMapAlbedo;
    ColorMap mColorMapRoughness;
    ColorMap mColorMapMetallic;
    ColorMap mColorMapAO;
    ColorMap mColorMapAmbient;
    ColorMap mColorMapEmissive;

    float mSpecularShininess;
  };

  Geometry();
  ~Geometry();

  bool LoadMesh( const char * _path );
  void UnloadMesh();

  void Render( const glm::mat4x4 & _worldRootMatrix, Renderer::Shader * _shader );

  void __SetupVertexArray( Renderer::Shader * _shader, const char * name, int sizeInFloats, int & offsetInFloats);
  void RebindVertexArray( Renderer::Shader * _shader );

  void SetColorMap( Renderer::Shader * _shader, const char * _name, const ColorMap & _colorMap );

  static std::string GetSupportedExtensions();

  std::map<int, Node> mNodes;
  std::map<int, Mesh> mMeshes;
  std::map<int, Material> mMaterials;
  glm::mat4x4 * mMatrices;
  glm::vec3 mAABBMin;
  glm::vec3 mAABBMax;
  glm::vec4 mGlobalAmbient;
};