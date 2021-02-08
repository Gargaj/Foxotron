#include "Geometry.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Exporter.hpp>

#include <glm.hpp>

Assimp::Importer gImporter;

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

std::map<int, Geometry::Node> Geometry::mNodes;
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

int gNodeCount = 0;
void ParseNode( const aiScene * scene, aiNode * sceneNode, int nParentIndex )
{
  Geometry::Node node;
  node.mID = gNodeCount++;
  node.mParentID = nParentIndex;

  for ( unsigned int i = 0; i < sceneNode->mNumMeshes; i++ )
  {
    node.mMeshes.push_back( sceneNode->mMeshes[ i ] );
  }

  aiMatrix4x4 m = sceneNode->mTransformation.Transpose();
  memcpy( &node.mTransformation, &m.a1, sizeof( float ) * 16 );

  Geometry::mNodes.insert( { node.mID, node } );

  for ( unsigned int i = 0; i < sceneNode->mNumChildren; i++ )
  {
    ParseNode( scene, sceneNode->mChildren[ i ], node.mID );
  }
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

  gImporter.SetPropertyInteger( AI_CONFIG_PP_SBBC_MAX_BONES, 24 );

  unsigned int loadFlags =
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

  const aiScene * scene = gImporter.ReadFile( _path, loadFlags );
  if ( !scene )
  {
    return false;
  }

  gNodeCount = 0;
  ParseNode( scene, scene->mRootNode, -1 );

  for ( unsigned int i = 0; i < scene->mNumMeshes; i++ )
  {
    aiMesh * sceneMesh = scene->mMeshes[ i ];

    if ( !sceneMesh->mNumVertices || !sceneMesh->mNumFaces )
    {
      continue;
    }

    Mesh mesh;
    mesh.mVertexCount = sceneMesh->mNumVertices;

    Vertex * vertices = new Vertex[ mesh.mVertexCount ];
    for ( unsigned int j = 0; j < sceneMesh->mNumVertices; j++ )
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

    for ( unsigned int j = 0; j < sceneMesh->mNumFaces; j++ )
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

  for ( unsigned int i = 0; i < scene->mNumMaterials; i++ )
  {
    Material material;

    aiString str;

    material.mTextureDiffuse = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureDiffuse = LoadTexture( str, folder );
    }
    material.mTextureNormals = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_NORMALS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureNormals = LoadTexture( str, folder );
    }
    material.mTextureSpecular = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_SPECULAR, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureSpecular = LoadTexture( str, folder );
    }
    material.mTextureAlbedo = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_BASE_COLOR, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureAlbedo = LoadTexture( str, folder );
    }
    material.mTextureRoughness = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE_ROUGHNESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureRoughness = LoadTexture( str, folder );
    }
    material.mTextureMetallic = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_METALNESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureMetallic = LoadTexture( str, folder );
    }

    mMaterials.insert( { i, material } );
  }


  return true;
}

void Geometry::UnloadMesh()
{
  mNodes.clear();

  for ( std::map<int, Material>::iterator it = mMaterials.begin(); it != mMaterials.end(); it++ )
  {
    if ( it->second.mTextureDiffuse )
    {
      Renderer::ReleaseTexture( it->second.mTextureDiffuse );
    }
    if ( it->second.mTextureNormals )
    {
      Renderer::ReleaseTexture( it->second.mTextureNormals );
    }
    if ( it->second.mTextureSpecular )
    {
      Renderer::ReleaseTexture( it->second.mTextureSpecular );
    }
    if ( it->second.mTextureAlbedo )
    {
      Renderer::ReleaseTexture( it->second.mTextureAlbedo );
    }
    if ( it->second.mTextureRoughness )
    {
      Renderer::ReleaseTexture( it->second.mTextureRoughness );
    }
    if ( it->second.mTextureMetallic )
    {
      Renderer::ReleaseTexture( it->second.mTextureMetallic );
    }
  }
  mMaterials.clear();

  for ( std::map<int, Mesh>::iterator it = mMeshes.begin(); it != mMeshes.end(); it++ )
  {
    glDeleteBuffers( 1, &it->second.mIndexBufferObject );
    glDeleteBuffers( 1, &it->second.mVertexBufferObject );
  }
  mMeshes.clear();

  gImporter.FreeScene();
}

std::string Geometry::GetSupportedExtensions()
{
  std::string out;
  gImporter.GetExtensionList( out );

  for ( int i = 0; i < out.length(); i++ )
  {
    if ( out[ i ] == '*' )
    {
      out = out.substr( 0, i ) + out.substr( i + 1 );
      i--;
    }
    else if ( out[ i ] == ';' )
    {
      out[ i ] = ',';
    }
  }

  return out;
}

