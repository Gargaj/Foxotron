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
#include <common.hpp>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

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

// Transform an AABB into an OBB, and return its AABB
void TransformBoundingBox( const glm::vec3 & inMin, const glm::vec3 & inMax, const glm::mat4x4 & m, glm::vec3 & outMin, glm::vec3 & outMax )
{
  static glm::vec3 xa; xa = glm::vec3( m[ 1 - 1 ][ 1 - 1 ], m[ 1 - 1 ][ 2 - 1 ], m[ 1 - 1 ][ 3 - 1 ] ) * inMin.x;
  static glm::vec3 xb; xb = glm::vec3( m[ 1 - 1 ][ 1 - 1 ], m[ 1 - 1 ][ 2 - 1 ], m[ 1 - 1 ][ 3 - 1 ] ) * inMax.x;

  static glm::vec3 ya; ya = glm::vec3( m[ 2 - 1 ][ 1 - 1 ], m[ 2 - 1 ][ 2 - 1 ], m[ 2 - 1 ][ 3 - 1 ] ) * inMin.y;
  static glm::vec3 yb; yb = glm::vec3( m[ 2 - 1 ][ 1 - 1 ], m[ 2 - 1 ][ 2 - 1 ], m[ 2 - 1 ][ 3 - 1 ] ) * inMax.y;

  static glm::vec3 za; za = glm::vec3( m[ 3 - 1 ][ 1 - 1 ], m[ 3 - 1 ][ 2 - 1 ], m[ 3 - 1 ][ 3 - 1 ] ) * inMin.z;
  static glm::vec3 zb; zb = glm::vec3( m[ 3 - 1 ][ 1 - 1 ], m[ 3 - 1 ][ 2 - 1 ], m[ 3 - 1 ][ 3 - 1 ] ) * inMax.z;

  outMin = glm::min( xa, xb ) + glm::min( ya, yb ) + glm::min( za, zb ) + glm::vec3( m[ 4 - 1 ][ 1 - 1 ], m[ 4 - 1 ][ 2 - 1 ], m[ 4 - 1 ][ 3 - 1 ] );
  outMax = glm::max( xa, xb ) + glm::max( ya, yb ) + glm::max( za, zb ) + glm::vec3( m[ 4 - 1 ][ 1 - 1 ], m[ 4 - 1 ][ 2 - 1 ], m[ 4 - 1 ][ 3 - 1 ] );
}

Renderer::Texture * LoadTexture( const char * _type, const aiString & _path, const std::string & _folder, const bool _loadAsSRGB = false )
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

  printf( "[geometry] Loading %s texture: '%s'\n", _type, filename.c_str() );

  Renderer::Texture * texture = Renderer::CreateRGBA8TextureFromFile( filename.c_str() );
  if ( texture )
  {
    return texture;
  }

  filename = _folder + filename;

  texture = Renderer::CreateRGBA8TextureFromFile( filename.c_str(), _loadAsSRGB );
  if ( texture )
  {
    return texture;
  }

  std::string extless = filename.substr( 0, filename.find_last_of( '.' ) );

  const char * extensions[] = { ".hdr", ".png", ".tga", ".jpg", ".jpeg", ".bmp", NULL };
  for ( int i = 0; extensions[ i ]; i++ )
  {
    std::string replacementFilename = extless + extensions[ i ];
    texture = Renderer::CreateRGBA8TextureFromFile( replacementFilename.c_str(), _loadAsSRGB );
    if ( texture )
    {
      printf( "[geometry] Replacement %s texture found: '%s'\n", _type, replacementFilename.c_str() );
      return texture;
    }
  }

  printf( "[geometry] WARNING: Texture loading (%s) failed: '%s'\n", _type, filename.c_str() );
  return NULL;
}

int gNodeCount = 0;
void ParseNode( Geometry * _geometry, const aiScene * scene, aiNode * sceneNode, int nParentIndex )
{
  Geometry::Node node;
  node.mID = gNodeCount++;
  node.mParentID = nParentIndex;
  node.mName = std::string( sceneNode->mName.data, sceneNode->mName.length );

  for ( unsigned int i = 0; i < sceneNode->mNumMeshes; i++ )
  {
    node.mMeshes.push_back( sceneNode->mMeshes[ i ] );
  }

  aiMatrix4x4 m = sceneNode->mTransformation.Transpose();
  memcpy( &node.mTransformation, &m.a1, sizeof( float ) * 16 );

  _geometry->mNodes.insert( { node.mID, node } );

  for ( unsigned int i = 0; i < sceneNode->mNumChildren; i++ )
  {
    ParseNode( _geometry, scene, sceneNode->mChildren[ i ], node.mID );
  }
}

class GeometryLogging : public Assimp::LogStream
{
public:
  void write( const char * message )
  {
    printf( "[assimp] %s", message );
  }
};

Geometry::Geometry()
  : mMatrices( NULL )
  , mAABBMin( 0.0f )
  , mAABBMax( 0.0f )
{
}

Geometry::~Geometry()
{
  UnloadMesh();
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
    aiProcess_FlipWindingOrder |
    aiProcess_TransformUVCoords |
    aiProcess_FlipUVs |
    aiProcess_SplitByBoneCount |
    0;

  Assimp::DefaultLogger::create( "", Assimp::Logger::DEBUGGING );
  Assimp::DefaultLogger::get()->attachStream( new GeometryLogging(), Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn );

  const aiScene * scene = gImporter.ReadFile( _path, loadFlags );
  if ( !scene )
  {
    return false;
  }

  Assimp::DefaultLogger::kill();

  gNodeCount = 0;
  ParseNode( this, scene, scene->mRootNode, -1 );

  mMatrices = mNodes.size() ? new glm::mat4x4[ mNodes.size() ] : nullptr;

  //////////////////////////////////////////////////////////////////////////
  // Calculate node transforms
  if ( mMatrices )
  {
    for ( std::map<int, Geometry::Node>::iterator it = mNodes.begin(); it != mNodes.end(); it++ )
    {
      const Geometry::Node & node = it->second;

      glm::mat4x4 matParent;
      if ( node.mParentID == -1 || !mMatrices )
      {
        matParent = glm::mat4x4( 1.0f );
      }
      else
      {
        matParent = mMatrices[ node.mParentID ];
      }

      mMatrices[ node.mID ] = matParent * node.mTransformation;
    }
  }

  printf( "[geometry] Loading %d meshes\n", scene->mNumMeshes );
  for ( unsigned int i = 0; i < scene->mNumMeshes; i++ )
  {
    aiMesh * sceneMesh = scene->mMeshes[ i ];

    if ( !sceneMesh->mNumVertices || !sceneMesh->mNumFaces )
    {
      continue;
    }

    Mesh mesh;

    glGenVertexArrays( 1, &mesh.mVertexArrayObject );
    glGenBuffers( 1, &mesh.mVertexBufferObject );
    glGenBuffers( 1, &mesh.mIndexBufferObject );

    glBindVertexArray( mesh.mVertexArrayObject );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.mVertexBufferObject );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.mIndexBufferObject );

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

      if ( j == 0 )
      {
        mesh.mAABBMin = mesh.mAABBMax = vertices[ j ].v3Vector;
      }
      else
      {
        mesh.mAABBMin = glm::min( mesh.mAABBMin, vertices[ j ].v3Vector );
        mesh.mAABBMax = glm::max( mesh.mAABBMax, vertices[ j ].v3Vector );
      }
    }

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

    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( unsigned int ) * sceneMesh->mNumFaces * 3, faces, GL_STATIC_DRAW );

    delete[] faces;

    mesh.mMaterialIndex = sceneMesh->mMaterialIndex;

    mMeshes.insert( { i, mesh } );
  }

  printf( "[geometry] Calculating AABB\n" );
  bool aabbSet = false;
  for ( std::map<int, Geometry::Node>::iterator it = mNodes.begin(); it != mNodes.end(); it++ )
  {
    const Geometry::Node & node = it->second;
    for ( int i = 0; i < it->second.mMeshes.size(); i++ )
    {
      const Geometry::Mesh & mesh = mMeshes[ it->second.mMeshes[ i ] ];

      glm::vec3 aabbMin;
      glm::vec3 aabbMax;
      TransformBoundingBox( mesh.mAABBMin, mesh.mAABBMax, mMatrices[ node.mID ], aabbMin, aabbMax );

      if ( !aabbSet )
      {
        mAABBMin = aabbMin;
        mAABBMax = aabbMax;
        aabbSet = true;
      }
      mAABBMin = glm::min( aabbMin, mAABBMin );
      mAABBMax = glm::max( aabbMax, mAABBMax );
    }
  }
  printf( "[geometry] Calculated AABB: (%.3f, %.3f, %.3f), (%.3f, %.3f, %.3f)\n", mAABBMin.x, mAABBMin.y, mAABBMin.z, mAABBMax.x, mAABBMax.y, mAABBMax.z );

  printf( "[geometry] Loading %d materials\n", scene->mNumMaterials );
  for ( unsigned int i = 0; i < scene->mNumMaterials; i++ )
  {
    Material material;

    aiString str = scene->mMaterials[ i ]->GetName();
    material.mName = std::string( str.data, str.length );
    printf( "[geometry] Loading material #%d: '%s'\n", i + 1, material.mName.c_str() );

    material.mTextureDiffuse = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureDiffuse = LoadTexture( "diffuse", str, folder, true );
    }
    material.mTextureNormals = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_NORMAL_CAMERA, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureNormals = LoadTexture( "normals", str, folder );
    }
    else if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_NORMALS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureNormals = LoadTexture( "normals", str, folder );
    }
    material.mTextureSpecular = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_SPECULAR, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureSpecular = LoadTexture( "specular", str, folder );
    }
    material.mTextureAlbedo = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_BASE_COLOR, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureAlbedo = LoadTexture( "albedo", str, folder, true );
    }
    material.mTextureRoughness = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_DIFFUSE_ROUGHNESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureRoughness = LoadTexture( "roughness", str, folder );
    }
    else if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_SHININESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureRoughness = LoadTexture( "roughness (from shininess)", str, folder );
    }
    material.mTextureMetallic = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_METALNESS, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureMetallic = LoadTexture( "metallic", str, folder );
    }
    material.mTextureAO = NULL;
    if ( aiGetMaterialString( scene->mMaterials[ i ], AI_MATKEY_TEXTURE( aiTextureType_AMBIENT_OCCLUSION, 0 ), &str ) == AI_SUCCESS )
    {
      material.mTextureAO = LoadTexture( "ao", str, folder );
    }

    float f = 0.0f;

    material.mSpecularShininess = 1.0f;
    if ( aiGetMaterialFloat( scene->mMaterials[ i ], AI_MATKEY_SHININESS, &f ) == AI_SUCCESS )
    {
      material.mSpecularShininess = f;
    }

    aiColor4D color;

		material.mColorAmbient = glm::vec4( 0.0f, 0.0f, 0.0f, 0.0f );
		if ( aiGetMaterialColor( scene->mMaterials[ i ], AI_MATKEY_COLOR_AMBIENT, &color ) == AI_SUCCESS )
		{
			memcpy( &material.mColorAmbient.x, &color.r, sizeof( float ) * 4 );
		}
		material.mColorDiffuse = glm::vec4( 0.5f, 0.5f, 0.5f, 0.5f );
		if ( aiGetMaterialColor( scene->mMaterials[ i ], AI_MATKEY_COLOR_DIFFUSE, &color ) == AI_SUCCESS )
		{
			memcpy( &material.mColorDiffuse.x, &color.r, sizeof( float ) * 4 );
		}
    material.mColorSpecular = glm::vec4( 1.0f, 1.0f, 1.0f, 1.0f );
    if ( aiGetMaterialColor( scene->mMaterials[ i ], AI_MATKEY_COLOR_SPECULAR, &color ) == AI_SUCCESS )
    {
      memcpy( &material.mColorSpecular.x, &color.r, sizeof( float ) * 4 );
    }

    mMaterials.insert( { i, material } );
  }


  return true;
}

void Geometry::UnloadMesh()
{
  if ( mMatrices )
  {
    delete[] mMatrices;
    mMatrices = NULL;
  }

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
    if ( it->second.mTextureAO )
    {
      Renderer::ReleaseTexture( it->second.mTextureAO );
    }
  }
  mMaterials.clear();

  for ( std::map<int, Mesh>::iterator it = mMeshes.begin(); it != mMeshes.end(); it++ )
  {
    glDeleteBuffers( 1, &it->second.mIndexBufferObject );
    glDeleteBuffers( 1, &it->second.mVertexBufferObject );
    glDeleteVertexArrays( 1, &it->second.mVertexArrayObject );
  }
  mMeshes.clear();

  gImporter.FreeScene();
}

void Geometry::Render( const glm::mat4x4 & _worldRootMatrix, Renderer::Shader * _shader )
{
  Renderer::SetShader( _shader );

  for ( std::map<int, Geometry::Node>::iterator it = mNodes.begin(); it != mNodes.end(); it++ )
  {
    const Geometry::Node & node = it->second;

    _shader->SetConstant( "mat_world", mMatrices[ node.mID ] * _worldRootMatrix );

    for ( int i = 0; i < it->second.mMeshes.size(); i++ )
    {
      const Geometry::Mesh & mesh = mMeshes[ it->second.mMeshes[ i ] ];
      const Geometry::Material & material = mMaterials[ mesh.mMaterialIndex ];

      _shader->SetConstant( "color_ambient", material.mColorAmbient );
      _shader->SetConstant( "color_diffuse", material.mColorDiffuse );
      _shader->SetConstant( "color_specular", material.mColorSpecular );
      _shader->SetConstant( "specular_shininess", material.mSpecularShininess );

      _shader->SetConstant( "has_tex_diffuse", material.mTextureDiffuse != NULL );
      _shader->SetConstant( "has_tex_normals", material.mTextureNormals != NULL );
      _shader->SetConstant( "has_tex_specular", material.mTextureSpecular != NULL );
      _shader->SetConstant( "has_tex_albedo", material.mTextureAlbedo != NULL );
      _shader->SetConstant( "has_tex_roughness", material.mTextureRoughness != NULL );
      _shader->SetConstant( "has_tex_metallic", material.mTextureMetallic != NULL );
      _shader->SetConstant( "has_tex_ao", material.mTextureAO != NULL );

      if ( material.mTextureDiffuse )
      {
        _shader->SetTexture( "tex_diffuse", material.mTextureDiffuse );
      }
      if ( material.mTextureNormals )
      {
        _shader->SetTexture( "tex_normals", material.mTextureNormals );
      }
      if ( material.mTextureSpecular )
      {
        _shader->SetTexture( "tex_specular", material.mTextureSpecular );
      }
      if ( material.mTextureAlbedo )
      {
        _shader->SetTexture( "tex_albedo", material.mTextureAlbedo );
      }
      if ( material.mTextureRoughness )
      {
        _shader->SetTexture( "tex_roughness", material.mTextureRoughness );
      }
      if ( material.mTextureMetallic )
      {
        _shader->SetTexture( "tex_metallic", material.mTextureMetallic );
      }
      if ( material.mTextureAO )
      {
        _shader->SetTexture( "tex_ao", material.mTextureAO );
      }

      glBindVertexArray( mesh.mVertexArrayObject );

      glDrawElements( GL_TRIANGLES, mesh.mTriangleCount * 3, GL_UNSIGNED_INT, NULL );
    }
  }
}

void Geometry::__SetupVertexArray( Renderer::Shader * _shader, const char * name, int sizeInFloats, int & offsetInFloats )
{
  unsigned int stride = sizeof( float ) * 14;

  GLint location = glGetAttribLocation( _shader->mProgram, name );
  if ( location >= 0 )
  {
    glVertexAttribPointer( location, sizeInFloats, GL_FLOAT, GL_FALSE, stride, (GLvoid *) ( offsetInFloats * sizeof( GLfloat ) ) );
    glEnableVertexAttribArray( location );
  }

  offsetInFloats += sizeInFloats;
}

void Geometry::RebindVertexArray( Renderer::Shader * _shader )
{
  for ( int i = 0; i < mMeshes.size(); i++ )
  {
    const Geometry::Mesh & mesh = mMeshes[ i ];

    glBindVertexArray( mesh.mVertexArrayObject );
    glBindBuffer( GL_ARRAY_BUFFER, mesh.mVertexBufferObject );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.mIndexBufferObject );

    int offset = 0;
    __SetupVertexArray( _shader, "in_pos", 3, offset );
    __SetupVertexArray( _shader, "in_normal", 3, offset );
    __SetupVertexArray( _shader, "in_tangent", 3, offset );
    __SetupVertexArray( _shader, "in_binormal", 3, offset );
    __SetupVertexArray( _shader, "in_texcoord", 2, offset );
  }
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
