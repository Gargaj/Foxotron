#include <map>

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

bool LoadMesh( const char * _path );
void UnloadMesh();

extern std::map<int, Mesh> mMeshes;
}