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

#include <glm.hpp>

Assimp::Importer mImporter;

#pragma pack(1)
struct Vertex
{
  glm::vec3 v3Vector;
  glm::vec3 v3Normal;
  glm::vec3 v3Tangent;
  glm::vec3 v3Binormal;
  glm::vec2 fTexcoord;
};
#pragma pack()

std::map<int, Geometry::Mesh> Geometry::mMeshes;

bool Geometry::LoadMesh( const char * _path )
{
  UnloadMesh();

  mImporter.SetPropertyInteger( AI_CONFIG_PP_SBBC_MAX_BONES, 24 );

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

  const aiScene * scene = mImporter.ReadFile( _path, dwLoadFlags );
  if ( !scene )
  {
    return false;
  }

  // TODO: load nodes

  for ( int i = 0; i < scene->mNumMeshes; i++ )
  {
    aiMesh * sceneMesh = scene->mMeshes[ i ];

    if ( !sceneMesh->mNumVertices || !sceneMesh->mNumFaces )
    {
      continue;
    }

    Mesh mesh;
    mesh.mVertexCount = sceneMesh->mNumVertices;

    Vertex * vertices = new Vertex[ mesh.mVertexCount ];
    for ( int j = 0; j < sceneMesh->mNumVertices; j++ )
    {
      vertices[ j ].v3Vector.x = sceneMesh->mVertices[ j ].x;
      vertices[ j ].v3Vector.y = sceneMesh->mVertices[ j ].y;
      vertices[ j ].v3Vector.z = sceneMesh->mVertices[ j ].z;
      vertices[ j ].v3Normal.x = sceneMesh->mNormals[ j ].x;
      vertices[ j ].v3Normal.y = sceneMesh->mNormals[ j ].y;
      vertices[ j ].v3Normal.z = sceneMesh->mNormals[ j ].z;
      vertices[ j ].v3Tangent.x = 0.0f;
      vertices[ j ].v3Tangent.y = 0.0f;
      vertices[ j ].v3Tangent.z = 0.0f;
      if ( sceneMesh->mTangents )
      {
        vertices[ j ].v3Tangent.x = sceneMesh->mTangents[ j ].x;
        vertices[ j ].v3Tangent.y = sceneMesh->mTangents[ j ].y;
        vertices[ j ].v3Tangent.z = sceneMesh->mTangents[ j ].z;
      }
      vertices[ j ].v3Binormal.x = 0.0f;
      vertices[ j ].v3Binormal.y = 0.0f;
      vertices[ j ].v3Binormal.z = 0.0f;
      if ( sceneMesh->mBitangents )
      {
        vertices[ j ].v3Binormal.x = sceneMesh->mBitangents[ j ].x;
        vertices[ j ].v3Binormal.y = sceneMesh->mBitangents[ j ].y;
        vertices[ j ].v3Binormal.z = sceneMesh->mBitangents[ j ].z;
      }
      if ( sceneMesh->GetNumUVChannels() )
      {
        vertices[ j ].fTexcoord.x = sceneMesh->mTextureCoords[ 0 ][ j ].x;
        vertices[ j ].fTexcoord.y = sceneMesh->mTextureCoords[ 0 ][ j ].y;
      }
      else
      {
        vertices[ j ].fTexcoord.x = 0.0f;
        vertices[ j ].fTexcoord.y = 0.0f;
      }
    }

    glGenBuffers( 1, &mesh.mVertexBufferObject );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.mVertexBufferObject );
    glBufferData( GL_ARRAY_BUFFER, sizeof( Vertex ) * mesh.mVertexCount, vertices, GL_STATIC_DRAW );

    delete[] vertices;

    mesh.mTriangleCount = sceneMesh->mNumFaces;
    
    unsigned int * p = new unsigned int[ sceneMesh->mNumFaces * 3 ];

    for ( int j = 0; j < sceneMesh->mNumFaces; j++ )
    {
      aiFace * faces = sceneMesh->mFaces;
      p[ j * 3 + 0 ] = sceneMesh->mFaces[ j ].mIndices[ 0 ];
      p[ j * 3 + 1 ] = sceneMesh->mFaces[ j ].mIndices[ 1 ];
      p[ j * 3 + 2 ] = sceneMesh->mFaces[ j ].mIndices[ 2 ];
    }

    glGenBuffers( 1, &mesh.mIndexBufferObject );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.mIndexBufferObject );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * sceneMesh->mNumFaces * 3, p, GL_STATIC_DRAW );

    delete[] p;

    mesh.mMaterialIndex = sceneMesh->mMaterialIndex;

    mMeshes.insert( { i, mesh } );
  }

  // TODO: load materials

  return true;
}

void Geometry::UnloadMesh()
{
  for ( std::map<int, Mesh>::iterator it = mMeshes.begin(); it != mMeshes.end(); it++ )
  {
    glDeleteBuffers( 1, &it->second.mIndexBufferObject );
    glDeleteBuffers( 1, &it->second.mVertexBufferObject );
  }
  mMeshes.clear();

  mImporter.FreeScene();
}

