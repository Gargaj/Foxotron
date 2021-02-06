#include "Geometry.h"

#include <assimp/scene.h>
#include <assimp/postProcess.h>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Exporter.hpp>

Assimp::Importer importer;

bool Geometry::LoadMesh( const char * _path )
{
  importer.SetPropertyInteger( AI_CONFIG_PP_SBBC_MAX_BONES, 24 );

  unsigned int dwLoadFlags =
    aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_JoinIdenticalVertices |
    aiProcess_SortByPType |
    aiProcess_MakeLeftHanded |
    aiProcess_FlipWindingOrder |
    aiProcess_TransformUVCoords |
    aiProcess_FlipUVs |
    aiProcess_SplitByBoneCount |
    0;

  const aiScene * scene = importer.ReadFile( _path, dwLoadFlags );
  if ( !scene )
  {
    return false;
  }

  return true;
}

void Geometry::UnloadMesh()
{
  importer.FreeScene();
}

