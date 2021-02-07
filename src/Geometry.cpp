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
std::map<int, Geometry::Material> Geometry::mMaterials;

Renderer::Texture * LoadTexture( const aiString & _path, const std::string & _folder )
{
  std::string filename( _path.data, _path.length );

  if ( filename.find( '\\' ) != -1 )
  {
    filename = filename.substr( filename.find_last_of( '\\' ) + 1 );
  }
  if ( filename.find( '/' ) != -1 )
  {
    filename = filename.substr( filename.find_last_of( '/' ) + 1 );
  }

  Renderer::Texture * texture = Renderer::CreateRGBA8TextureFromFile( filename.c_str() );
  if ( texture )
  {
    return texture;
  }

  filename = _folder + filename;

  texture = Renderer::CreateRGBA8TextureFromFile( filename.c_str() );
  if ( texture )
  {
    return texture;
  }

  return NULL;
}

bool Geometry::LoadMesh( const char * _path )
{
  UnloadMesh();

  std::string path = _path;
  std::string folder;
  if ( path.find( '\\' ) != -1 )
  {
    folder = path.substr( 0, path.find_last_of( '\\' ) + 1 );
  }
  if ( path.find( '/' ) != -1 )
  {
    folder = path.substr( 0, path.find_last_of( '/' ) + 1 );
  }

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
    
    unsigned int * faces = new unsigned int[ sceneMesh->mNumFaces * 3 ];

    for ( int j = 0; j < sceneMesh->mNumFaces; j++ )
    {
      faces[ j * 3 + 0 ] = sceneMesh->mFaces[ j ].mIndices[ 0 ];
      faces[ j * 3 + 1 ] = sceneMesh->mFaces[ j ].mIndices[ 1 ];
      faces[ j * 3 + 2 ] = sceneMesh->mFaces[ j ].mIndices[ 2 ];
    }

    glGenBuffers( 1, &mesh.mIndexBufferObject );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.mIndexBufferObject );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * sceneMesh->mNumFaces * 3, faces, GL_STATIC_DRAW );

    delete[] faces;

    mesh.mMaterialIndex = sceneMesh->mMaterialIndex;

    mMeshes.insert( { i, mesh } );
  }

  for ( int i = 0; i < scene->mNumMaterials; i++ )
  {
    Material material;

    aiString str;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE, 0 ), &str ) == AI_SUCCESS )
    {
      material.textureDiffuse = LoadTexture( str, folder );
    }
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_NORMALS, 0 ), &str ) == AI_SUCCESS )
    {
      material.textureNormals = LoadTexture( str, folder );
    }
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_SPECULAR, 0 ), &str ) == AI_SUCCESS )
    {
      material.textureSpecular = LoadTexture( str, folder );
    }
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_BASE_COLOR, 0 ), &str ) == AI_SUCCESS )
    {
      material.textureAlbedo = LoadTexture( str, folder );
    }
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE_ROUGHNESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.textureRoughness = LoadTexture( str, folder );
    }
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_METALNESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.textureMetallic = LoadTexture( str, folder );
    }

    mMaterials.insert( { i, material } );
  }


  return true;
}

void Geometry::UnloadMesh()
{
  for ( std::map<int, Material>::iterator it = mMaterials.begin(); it != mMaterials.end(); it++ )
  {
    if ( it->second.textureDiffuse )
    {
      Renderer::ReleaseTexture( it->second.textureDiffuse );
    }
    if ( it->second.textureNormals )
    {
      Renderer::ReleaseTexture( it->second.textureNormals );
    }
    if ( it->second.textureSpecular )
    {
      Renderer::ReleaseTexture( it->second.textureSpecular );
    }
    if ( it->second.textureAlbedo )
    {
      Renderer::ReleaseTexture( it->second.textureAlbedo );
    }
    if ( it->second.textureRoughness )
    {
      Renderer::ReleaseTexture( it->second.textureRoughness );
    }
    if ( it->second.textureMetallic )
    {
      Renderer::ReleaseTexture( it->second.textureMetallic );
    }
  }
  for ( std::map<int, Mesh>::iterator it = mMeshes.begin(); it != mMeshes.end(); it++ )
  {
    glDeleteBuffers( 1, &it->second.mIndexBufferObject );
    glDeleteBuffers( 1, &it->second.mVertexBufferObject );
  }
  mMeshes.clear();

  mImporter.FreeScene();
}

