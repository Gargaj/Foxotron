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

  Renderer::Texture * texture = Renderer::CreateRGBA8TextureFromFile( filename.c_str(), _loadAsSRGB );
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

bool LoadColorMap( aiMaterial * _material, Geometry::ColorMap & _colorMap, aiTextureType _semantic, const char * _semanticText, const std::string & _folder, bool _loadAsSRGB = false )
{
  bool success = false;
  _colorMap.mTexture = NULL;

  aiString str;
  if ( aiGetMaterialString( _material, AI_MATKEY_TEXTURE( _semantic, 0 ), &str ) == AI_SUCCESS )
  {
    _colorMap.mTexture = LoadTexture( _semanticText, str, _folder, _loadAsSRGB );
    _colorMap.mValid = true;
    success = true;
  }

  aiColor4D color;
  aiColor4D translucentColor;
  aiReturn result = AI_FAILURE, result2 = AI_FAILURE;
  switch ( _semantic )
  {
    case aiTextureType_AMBIENT:
      result = aiGetMaterialColor( _material, AI_MATKEY_COLOR_AMBIENT, &color );
      break;
    case aiTextureType_DIFFUSE:
    case aiTextureType_BASE_COLOR:
       result = aiGetMaterialColor( _material, AI_MATKEY_COLOR_DIFFUSE, &color );
      result2 = aiGetMaterialColor( _material, AI_MATKEY_COLOR_TRANSPARENT, &translucentColor );
      if (result2 == AI_SUCCESS)
        color.a = (1.0 - translucentColor.r);
      break;
    case aiTextureType_SPECULAR:
      result = aiGetMaterialColor( _material, AI_MATKEY_COLOR_SPECULAR, &color );
      break;
    case aiTextureType_EMISSIVE:
      result = aiGetMaterialColor( _material, AI_MATKEY_COLOR_EMISSIVE, &color );
      break;
  };
  if ( result == AI_SUCCESS )
  {
    memcpy( &_colorMap.mColor, &color.r, sizeof( float ) * 4 );
    _colorMap.mValid = true;
  }

  return success;
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

  printf("[geometry] Loading %d materials\n", scene->mNumMaterials);
  for (unsigned int i = 0; i < scene->mNumMaterials; i++)
  {
    Material material;

    aiString str = scene->mMaterials[i]->GetName();
    material.mName = std::string(str.data, str.length);
    printf("[geometry] Loading material #%d: '%s'\n", i + 1, material.mName.c_str());

    material.mColorMapDiffuse.mColor = glm::vec4(0.5f);
    material.mColorMapNormals.mColor = glm::vec4(0.0f);
    material.mColorMapSpecular.mColor = glm::vec4(0.0f);
    material.mColorMapAlbedo.mColor = glm::vec4(0.5f);
    material.mColorMapRoughness.mColor = glm::vec4(1.0f);
    material.mColorMapMetallic.mColor = glm::vec4(0.0f);
    material.mColorMapAO.mColor = glm::vec4(1.0f);
    material.mColorMapAmbient.mColor = glm::vec4(1.0f);
    material.mColorMapEmissive.mColor = glm::vec4(0.0f);

    LoadColorMap(scene->mMaterials[i], material.mColorMapDiffuse, aiTextureType_DIFFUSE, "diffuse", folder, true);
    if (!LoadColorMap(scene->mMaterials[i], material.mColorMapNormals, aiTextureType_NORMAL_CAMERA, "normals", folder))
    {
      LoadColorMap(scene->mMaterials[i], material.mColorMapNormals, aiTextureType_NORMALS, "normals", folder);
    }
    LoadColorMap(scene->mMaterials[i], material.mColorMapSpecular, aiTextureType_SPECULAR, "specular", folder);
    LoadColorMap(scene->mMaterials[i], material.mColorMapAlbedo, aiTextureType_BASE_COLOR, "albedo", folder);
    if (!LoadColorMap(scene->mMaterials[i], material.mColorMapRoughness, aiTextureType_DIFFUSE_ROUGHNESS, "roughness", folder))
    {
      LoadColorMap(scene->mMaterials[i], material.mColorMapRoughness, aiTextureType_SHININESS, "roughness (from shininess)", folder);
    }
    LoadColorMap(scene->mMaterials[i], material.mColorMapMetallic, aiTextureType_METALNESS, "metallic", folder);
    LoadColorMap(scene->mMaterials[i], material.mColorMapAO, aiTextureType_AMBIENT_OCCLUSION, "AO", folder);
    LoadColorMap(scene->mMaterials[i], material.mColorMapAmbient, aiTextureType_AMBIENT, "ambient", folder);
    LoadColorMap(scene->mMaterials[i], material.mColorMapEmissive, aiTextureType_EMISSIVE, "emissive", folder);

    float f = 0.0f;

    material.mSpecularShininess = 1.0f;
    if (aiGetMaterialFloat(scene->mMaterials[i], AI_MATKEY_SHININESS, &f) == AI_SUCCESS)
    {
      material.mSpecularShininess = f;
    }

    mMaterials.insert({ i, material });
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

    // By importing materials before meshes we can investigate whether a mesh is transparent and flag it as such.
    const Geometry::Material& mtl = mMaterials[mesh.mMaterialIndex];
    mesh.mTransparent = (mtl.mColorMapAlbedo.mTexture != nullptr) ? (mtl.mColorMapAlbedo.mTexture->mTransparent) : (mtl.mColorMapAlbedo.mColor.a != 1.0f);

    mMeshes.insert({ i, mesh });
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

  mGlobalAmbient = glm::vec4( 0.3f );
  for ( unsigned int i = 0; i < scene->mNumLights; i++ )
  {
    switch ( scene->mLights[ i ]->mType )
    {
      case aiLightSource_AMBIENT:
        {
          memcpy( &mGlobalAmbient, &scene->mLights[ i ]->mColorAmbient.r, sizeof( float ) * 4 );
        } break;
      default:
        {
          // todo
        } break;
    }
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
    if ( it->second.mColorMapDiffuse.mTexture)
    {
      Renderer::ReleaseTexture( it->second.mColorMapDiffuse.mTexture);
    }
    if ( it->second.mColorMapNormals.mTexture)
    {
      Renderer::ReleaseTexture( it->second.mColorMapNormals.mTexture);
    }
    if ( it->second.mColorMapSpecular.mTexture)
    {
      Renderer::ReleaseTexture( it->second.mColorMapSpecular.mTexture);
    }
    if ( it->second.mColorMapAlbedo.mTexture)
    {
      Renderer::ReleaseTexture( it->second.mColorMapAlbedo.mTexture);
    }
    if ( it->second.mColorMapRoughness.mTexture)
    {
      Renderer::ReleaseTexture( it->second.mColorMapRoughness.mTexture);
    }
    if ( it->second.mColorMapMetallic.mTexture)
    {
      Renderer::ReleaseTexture( it->second.mColorMapMetallic.mTexture);
    }
    if ( it->second.mColorMapAO.mTexture )
    {
      Renderer::ReleaseTexture( it->second.mColorMapAO.mTexture );
    }
    if ( it->second.mColorMapAmbient.mTexture )
    {
      Renderer::ReleaseTexture( it->second.mColorMapAmbient.mTexture );
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

  _shader->SetConstant( "global_ambient", mGlobalAmbient );
  
  // TODO: Maybe we can cache the world matrices & 2 queues for opaque and transparant in LoadMesh
  // but that depends if we want to add animation support (in which case we can't).
  for(int j = 0; j < 2; ++j) // loop twice, first opaque, then transparent
  {
    bool bTransparentPass = (bool)j;
    if(bTransparentPass) {
      glEnable( GL_BLEND );
      glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
      glDepthMask( GL_FALSE);
    }
    for ( std::map<int, Geometry::Node>::iterator it = mNodes.begin(); it != mNodes.end(); it++ )
    {
      const Geometry::Node & node = it->second;
    
      _shader->SetConstant( "mat_world", mMatrices[ node.mID ] * _worldRootMatrix );
    
      for ( int i = 0; i < it->second.mMeshes.size(); i++ )
      {
        const Geometry::Mesh & mesh = mMeshes[ it->second.mMeshes[ i ] ];
        // Postpone rendering transparent meshes
        if(mesh.mTransparent != bTransparentPass)
          continue;
        const Geometry::Material & material = mMaterials[ mesh.mMaterialIndex ];
    
        _shader->SetConstant( "specular_shininess", material.mSpecularShininess );
    
        SetColorMap( _shader, "map_diffuse", material.mColorMapDiffuse );
        SetColorMap( _shader, "map_normals", material.mColorMapNormals );
        SetColorMap( _shader, "map_specular", material.mColorMapSpecular );
        SetColorMap( _shader, "map_albedo", material.mColorMapAlbedo );
        SetColorMap( _shader, "map_roughness", material.mColorMapRoughness );
        SetColorMap( _shader, "map_metallic", material.mColorMapMetallic );
        SetColorMap( _shader, "map_ao", material.mColorMapAO );
        SetColorMap( _shader, "map_ambient", material.mColorMapAmbient );
        SetColorMap( _shader, "map_emissive", material.mColorMapEmissive );
    
        glBindVertexArray( mesh.mVertexArrayObject );
    
        glDrawElements( GL_TRIANGLES, mesh.mTriangleCount * 3, GL_UNSIGNED_INT, NULL );
      }
    }
    if (bTransparentPass) {
      glDisable( GL_BLEND );
      glDepthMask( GL_TRUE );
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

void Geometry::SetColorMap( Renderer::Shader * _shader, const char * _name, const ColorMap & _colorMap )
{
  char sz[ 64 ];

  snprintf( sz, 64, "%s.color", _name );
  _shader->SetConstant( sz, _colorMap.mColor );

  snprintf( sz, 64, "%s.has_tex", _name );
  _shader->SetConstant( sz, _colorMap.mTexture != NULL );

  if ( _colorMap.mTexture )
  {
    snprintf( sz, 64, "%s.tex", _name );
    _shader->SetTexture( sz, _colorMap.mTexture );
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
