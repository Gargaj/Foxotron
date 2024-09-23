#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include "Geometry.h"
#include "SetupDialog.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "ImGuiFileBrowser.h"

#include "ext.hpp"
#include "gtx/rotate_vector.hpp"

#include <jsonxx.h>

Renderer::Shader * LoadShader( const char * vsPath, const char * fsPath )
{
  char vertexShader[ 16 * 1024 ] = { 0 };
  FILE * fileVS = fopen( vsPath, "rb" );
  if ( !fileVS )
  {
    printf( "Vertex shader load failed: '%s'\n", vsPath );
    return NULL;
  }
  fread( vertexShader, 1, 16 * 1024, fileVS );
  fclose( fileVS );

  char fragmentShader[ 16 * 1024 ] = { 0 };
  FILE * fileFS = fopen( fsPath, "rb" );
  if ( !fileFS )
  {
    printf( "Fragment shader load failed: '%s'\n", fsPath );
    return NULL;
  }
  fread( fragmentShader, 1, 16 * 1024, fileFS );
  fclose( fileFS );

  char error[ 4096 ];
  Renderer::Shader * shader = Renderer::CreateShader( vertexShader, (int) strlen( vertexShader ), fragmentShader, (int) strlen( fragmentShader ), error, 4096 );
  if ( !shader )
  {
    printf( "Shader build failed: %s\n", error );
  }

  return shader;
}

const jsonxx::Object * gCurrentSkyImageConfig = NULL;
const jsonxx::Object * gCurrentShaderConfig = NULL;
Renderer::Shader * gCurrentShader = NULL;
bool LoadShaderConfig( const jsonxx::Object * _shader )
{
  Renderer::Shader * newShader = LoadShader( _shader->get<jsonxx::String>( "vertexShader" ).c_str(), _shader->get<jsonxx::String>( "fragmentShader" ).c_str() );
  if ( !newShader )
  {
    return false;
  }

  gCurrentShaderConfig = _shader;

  if ( gCurrentShader )
  {
    Renderer::ReleaseShader( gCurrentShader );
    delete gCurrentShader;
  }
  gCurrentShader = newShader;

  return true;
}

jsonxx::Object gOptions;

glm::vec3 gCameraTarget( 0.0f, 0.0f, 0.0f );
float gCameraDistance = 500.0f;
Geometry gModel;

float gCameraYaw = glm::pi<float>() / 4.0f;
float gCameraPitch = 0.25f;
float gLightYaw = 0.0f;
float gLightPitch = 0.0f;
float gSkysphereOpacity = 1.0f;
float gSkysphereBlur = 0.0f;
glm::vec4 gClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
void LoadSkyImageConfig( const jsonxx::Object & obj );

void LoadMeshConfig( const char * path )
{
  FILE * configFile = fopen( path, "rb" );
  if ( !configFile )
  {
    return;
  }

  char meshconfigString[ 4096 ] = { 0 };
  memset( meshconfigString, 0, 4096 );
  fread( meshconfigString, 1, 4096, configFile );
  fclose( configFile );

  jsonxx::Object meshconfigRoot;
  if ( !meshconfigRoot.parse( meshconfigString ) )
  {
    return;
  }
  if ( !meshconfigRoot.has<jsonxx::Object>( "config" ) )
  {
    return;
  }

  const jsonxx::Object & meshconfig = meshconfigRoot.get<jsonxx::Object>( "config" );
  if ( meshconfig.has<jsonxx::String>( "shader" ) )
  {
    const std::string & shaderName = meshconfig.get<jsonxx::String>( "shader" );
    const int shaderCount = gOptions.get<jsonxx::Array>( "shaders" ).size();
    for ( int i = 0; i < shaderCount; i++ )
    {
      const jsonxx::Object & shaderConfig = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( i );
      if ( shaderConfig.get<jsonxx::String>( "name" ) == shaderName )
      {
        LoadShaderConfig( &shaderConfig );
        gModel.RebindVertexArray( gCurrentShader );
        break;
      }
    }
  }
  if ( meshconfig.has<jsonxx::String>( "skyImage" ) )
  {
    const std::string & skyImageName = meshconfig.get<jsonxx::String>( "skyImage" );
    const int skyImageCount = gOptions.get<jsonxx::Array>( "skyImages" ).size();
    for ( int i = 0; i < skyImageCount; i++ )
    {
      const jsonxx::Object & skyImageConfig = gOptions.get<jsonxx::Array>( "skyImages" ).get<jsonxx::Object>( i );
      if ( skyImageConfig.get<jsonxx::String>( "reflection" ) == skyImageName )
      {
        LoadSkyImageConfig( skyImageConfig );
        break;
      }
    }
  }
  if ( meshconfig.has<jsonxx::Number>( "cameraDistance" ) )
  {
    gCameraDistance = (float) meshconfig.get<jsonxx::Number>( "cameraDistance" );
  }
  if ( meshconfig.has<jsonxx::Number>( "cameraYaw" ) )
  {
    gCameraYaw = (float) meshconfig.get<jsonxx::Number>( "cameraYaw" );
  }
  if ( meshconfig.has<jsonxx::Number>( "cameraPitch" ) )
  {
    gCameraPitch = (float) meshconfig.get<jsonxx::Number>( "cameraPitch" );
  }
  if ( meshconfig.has<jsonxx::Number>( "lightYaw" ) )
  {
    gLightYaw = (float) meshconfig.get<jsonxx::Number>( "lightYaw" );
  }
  if ( meshconfig.has<jsonxx::Number>( "lightPitch" ) )
  {
    gLightPitch = (float) meshconfig.get<jsonxx::Number>( "lightPitch" );
  }
  if ( meshconfig.has<jsonxx::Number>( "skysphereOpacity" ) )
  {
    gSkysphereOpacity = (float) meshconfig.get<jsonxx::Number>( "skysphereOpacity" );
  }
  if ( meshconfig.has<jsonxx::Number>( "skysphereBlur" ) )
  {
    gSkysphereBlur = (float) meshconfig.get<jsonxx::Number>( "skysphereBlur" );
  }
  if ( meshconfig.has<jsonxx::Array>( "clearColor" ) )
  {
    gClearColor.x = (float) meshconfig.get<jsonxx::Array>( "clearColor" ).get<jsonxx::Number>( 0 );
    gClearColor.y = (float) meshconfig.get<jsonxx::Array>( "clearColor" ).get<jsonxx::Number>( 1 );
    gClearColor.z = (float) meshconfig.get<jsonxx::Array>( "clearColor" ).get<jsonxx::Number>( 2 );
    gClearColor.w = (float) meshconfig.get<jsonxx::Array>( "clearColor" ).get<jsonxx::Number>( 3 );
  }
  if ( meshconfig.has<jsonxx::Array>( "cameraTarget" ) )
  {
    gCameraTarget.x = (float) meshconfig.get<jsonxx::Array>( "cameraTarget" ).get<jsonxx::Number>( 0 );
    gCameraTarget.y = (float) meshconfig.get<jsonxx::Array>( "cameraTarget" ).get<jsonxx::Number>( 1 );
    gCameraTarget.z = (float) meshconfig.get<jsonxx::Array>( "cameraTarget" ).get<jsonxx::Number>( 2 );
  }
}

void SaveMeshConfig( const char * path )
{
  FILE * configFile = fopen( path, "wb" );
  if ( !configFile )
  {
    printf( "Unable to write mesh config file '%s'\n", path );
    return;
  }

  jsonxx::Object meshconfig;

  meshconfig << "shader" << gCurrentShaderConfig->get<jsonxx::String>( "name" );
  meshconfig << "skyImage" << gCurrentSkyImageConfig->get<jsonxx::String>( "reflection" );
  meshconfig << "cameraDistance" << gCameraDistance;
  meshconfig << "cameraYaw" << gCameraYaw;
  meshconfig << "cameraPitch" << gCameraPitch;
  meshconfig << "lightYaw" << gLightYaw;
  meshconfig << "lightPitch" << gLightPitch;
  meshconfig << "skysphereOpacity" << gSkysphereOpacity;
  meshconfig << "skysphereBlur" << gSkysphereBlur;

  jsonxx::Array clearColorArray;
  clearColorArray << gClearColor.x;
  clearColorArray << gClearColor.y;
  clearColorArray << gClearColor.z;
  clearColorArray << gClearColor.w;
  meshconfig << "clearColor" << clearColorArray;

  jsonxx::Array cameraTargetArray;
  cameraTargetArray << gCameraTarget.x;
  cameraTargetArray << gCameraTarget.y;
  cameraTargetArray << gCameraTarget.z;
  meshconfig << "cameraTarget" << cameraTargetArray;

  jsonxx::Object meshconfigRoot;
  meshconfigRoot << "config" << meshconfig;

  std::string meshconfigString = meshconfigRoot.json();
  fwrite( meshconfigString.c_str(), 1, meshconfigString.length(), configFile );
  fclose( configFile );

  printf( "Saved mesh config file to '%s'\n", path );
}

std::string gMeshPath;
bool LoadMesh( const char * path )
{
  if ( !gModel.LoadMesh( path ) )
  {
    return false;
  }

  gMeshPath = path;
  gModel.RebindVertexArray( gCurrentShader );

  gCameraTarget = ( gModel.mAABBMin + gModel.mAABBMax ) / 2.0f;
  gCameraDistance = glm::length( gCameraTarget - gModel.mAABBMin ) * 4.0f;

  char meshConfigPath[ 512 ];
  snprintf( meshConfigPath, 512, "%s.foxocfg", gMeshPath.c_str() );
  LoadMeshConfig( meshConfigPath );

  return true;
}

void ShowNodeInImGui( int _parentID )
{
  for ( std::map<int, Geometry::Node>::iterator it = gModel.mNodes.begin(); it != gModel.mNodes.end(); it++ )
  {
    if ( it->second.mParentID == _parentID )
    {
      ImGui::Text( "%s", it->second.mName.c_str() );
      ImGui::Indent();
      for ( int i = 0; i < it->second.mMeshes.size(); i++ )
      {
        const Geometry::Mesh & mesh = gModel.mMeshes[ it->second.mMeshes[ i ] ];
        ImGui::TextColored( ImVec4( 1.0f, 0.5f, 1.0f, 1.0f ), "Mesh %d: %d vertices, %d triangles", i + 1, mesh.mVertexCount, mesh.mTriangleCount );
        ImGui::SameLine();
        ImGui::TextColored( ImVec4( 1.0f, 0.75f, 1.0f, 1.0f ), "Material: %s", gModel.mMaterials[ mesh.mMaterialIndex ].mName.c_str() );        
      }

      ShowNodeInImGui( it->second.mID );
      ImGui::Unindent();
    }
  }
}

void ShowColorMapInImGui( const char * _channel, Geometry::ColorMap & _colorMap )
{
  if ( !_colorMap.mValid )
  {
    return;
  }

  if ( ImGui::BeginTabItem( _channel ) )
  {
    ImGui::ColorEdit4( "Color", (float *) &_colorMap.mColor, ImGuiColorEditFlags_AlphaPreviewHalf );
    if ( _colorMap.mTexture )
    {
      ImGui::Text( "Texture: %s", _colorMap.mTexture->mFilename.c_str() );
      ImGui::Text( "Is transparent: %s", _colorMap.mTexture->mTransparent ? "yes" : "no" );
      ImGui::Text( "Is SRGB: %s", _colorMap.mTexture->mSRGB ? "yes" : "no" );
      ImGui::Text( "Dimensions: %d x %d", _colorMap.mTexture->mWidth, _colorMap.mTexture->mHeight );
      ImGui::Image( (void *) (intptr_t) _colorMap.mTexture->mGLTextureID, ImVec2( 512.0f, 512.0f ) );
    }
    ImGui::EndTabItem();
  }
}

Renderer::Texture* gBrdfLookupTable = NULL;

void loadBrdfLookupTable()
{
  const int width = 256;
  const int height = 256;
  const int comp = 2;
  const char* filename = "Skyboxes/brdf256.bin";

  if ( gBrdfLookupTable )
  {
    Renderer::ReleaseTexture( gBrdfLookupTable );
    delete gBrdfLookupTable;
    gBrdfLookupTable = NULL;
  }

  gBrdfLookupTable = Renderer::CreateRG32FTextureFromRawFile( filename, width, height );

  if ( !gBrdfLookupTable )
  {
    printf( "Couldn't load %dx%d BRDF lookup table '%s'!\n", width, height, filename );
  }
}

// Reads a single string value from an HDRLabs IBL file.
// Returns an empty string on error.
static std::string parseIblField( const char* szPath, const char* szTargetSection, const char* szTargetKey )
{
  FILE* fp = fopen( szPath, "r" );
  if ( !fp ) return "";

  char section[ 100 ] = { '\0' };
  std::string found;

  while ( true )
  {
    char key[ 100 ] = { '\0' };
    char value[ 100 ] = { '\0' };

    int sectionRead = fscanf( fp, "[%[^]]\n", section );
    int keyValueRead = fscanf( fp, "%s = %s\n", key, value );

    if ( sectionRead == EOF || keyValueRead == EOF )
    {
      break;
    }

    if ( keyValueRead < 2 ) continue;

    if ( !strncmp( section, szTargetSection, sizeof( section ) ) && !strncmp( key, szTargetKey, sizeof( key ) ) )
    {
      found = value;
      break;
    }
  }

  fclose( fp );
  return found;
}

struct SkyImages
{
  Renderer::Texture* reflection = NULL;
  Renderer::Texture* env = NULL;
  float sunYaw = 0.f;
  float sunPitch = 0.f;
  glm::vec3 sunColor{ 1.f, 1.f, 1.f };
};
SkyImages gCurrentSkyImage;

void LoadSkyImageConfig( const jsonxx::Object & obj )
{
  gCurrentSkyImageConfig = &obj;

  const char* reflectionPath = obj.get<jsonxx::String>( "reflection" ).c_str();

  // Reflection map

  if ( gCurrentSkyImage.reflection )
  {
    Renderer::ReleaseTexture( gCurrentSkyImage.reflection );
    gCurrentSkyImage.reflection = NULL;
  }
  gCurrentSkyImage.reflection = Renderer::CreateRGBA8TextureFromFile( reflectionPath );

  if ( gCurrentSkyImage.reflection )
  {
      glBindTexture( GL_TEXTURE_2D, gCurrentSkyImage.reflection->mGLTextureID );

      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
  }

  if ( gCurrentSkyImage.env )
  {
    Renderer::ReleaseTexture( gCurrentSkyImage.env );
    gCurrentSkyImage.env = NULL;
  }

  // Irradiance map

  if ( obj.has<jsonxx::String>( "env" ) )
  {
    const char* envPath =  obj.get<jsonxx::String>( "env" ).c_str();
    gCurrentSkyImage.env = Renderer::CreateRGBA8TextureFromFile( envPath );

    if ( gCurrentSkyImage.env )
    {
      glBindTexture( GL_TEXTURE_2D, gCurrentSkyImage.env->mGLTextureID );

      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    }
    else
    {
      printf( "Couldn't load environment map '%s'!\n", envPath );
    }
  }

  // Sun direction

  if ( obj.has<jsonxx::String>( "metadata" ) )
  {
    const char* iblPath = obj.get<jsonxx::String>( "metadata" ).c_str();
    glm::vec2 uv(
      atof( parseIblField( iblPath, "Sun", "SUNu" ).c_str() ),
      atof( parseIblField( iblPath, "Sun", "SUNv" ).c_str() ) );

    if ( uv != glm::vec2( 0.f, 0.f ) )
    {
      gCurrentSkyImage.sunYaw = M_PI * (-2. * uv.x + 0.5f);
      gCurrentSkyImage.sunPitch = (.5f - uv.y) * M_PI;
    }
  }
}

int main( int argc, const char * argv[] )
{
  FILE * configFile = fopen( "config.json", "rb" );
  if ( !configFile )
  {
    printf( "Config file not found!\n" );
    return -10;
  }

  char configData[ 4096 ];
  memset( configData, 0, 4096 );
  fread( configData, 1, 4096, configFile );
  fclose( configFile );

  gOptions.parse( configData );
  if ( !gOptions.has<jsonxx::Array>( "shaders" ) || !gOptions.has<jsonxx::Array>( "skyImages" ) )
  {
    printf( "Config file broken!\n" );
    return -11;
  }

  //////////////////////////////////////////////////////////////////////////
  // Init renderer
  RENDERER_SETTINGS settings;
  settings.mVsync = false;
  settings.mWidth = 1280;
  settings.mHeight = 720;
  settings.mWindowMode = RENDERER_WINDOWMODE_WINDOWED;
  settings.mMultisampling = false;
#ifndef _DEBUG
  settings.mWidth = 1920; // TODO maybe replace this with actual screen size?
  settings.mHeight = 1080;
  settings.mWindowMode = RENDERER_WINDOWMODE_FULLSCREEN;
  settings.mMultisampling = true;
  if ( !SetupDialog::Open( &settings ) )
  {
    return -14;
  }
#endif

  if ( !Renderer::Open( &settings ) )
  {
    printf( "Renderer::Open failed\n" );
    return -1;
  }

  //////////////////////////////////////////////////////////////////////////
  // Start up ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO & io = ImGui::GetIO();

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL( Renderer::mWindow, true );
  ImGui_ImplOpenGL3_Init();

  imgui_addons::ImGuiFileBrowser file_dialog;

  //////////////////////////////////////////////////////////////////////////
  // Bootstrap
  if ( !LoadShaderConfig( &gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( 0 ) ) )
  {
    return -4;
  }

  auto firstSkyImageConfig = gOptions.get<jsonxx::Array>( "skyImages" ).get<jsonxx::Object>( 0 );
  LoadSkyImageConfig( firstSkyImageConfig );
  gLightYaw = gCurrentSkyImage.sunYaw;
  gLightPitch = gCurrentSkyImage.sunPitch;

  if ( argc >= 2 )
  {
    LoadMesh( argv[ 1 ] );
  }

  //////////////////////////////////////////////////////////////////////////
  // Mainloop
  bool appWantsToQuit = false;
  bool automaticCamera = false;
  uint32_t frameCount = 0;
  glm::mat4x4 viewMatrix;
  glm::mat4x4 projectionMatrix;
  bool rotatingCamera = false;
  bool movingCamera = false;
  bool movingLight = false;
  float mouseClickPosX = 0.0f;
  float mouseClickPosY = 0.0f;
  std::string supportedExtensions = Geometry::GetSupportedExtensions();
  float exposure = 1.0f;
  bool showImGui = true;
  bool edgedFaces = false;
  float hideCursorTimer = 0.0f;
  bool showModelInfo = false;
  bool xzySpace = false;
  const glm::mat4x4 xzyMatrix(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f,-1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f );

  loadBrdfLookupTable();

  Geometry skysphere;
  skysphere.LoadMesh( "Skyboxes/skysphere.fbx" );

  Renderer::Shader * skysphereShader = LoadShader( "Skyboxes/skysphere.vs", "Skyboxes/skysphere.fs" );
  if ( !skysphereShader )
  {
    return -8;
  }
  skysphere.RebindVertexArray( skysphereShader );

  while ( !Renderer::WantsToQuit() && !appWantsToQuit )
  {
    Renderer::StartFrame( gClearColor );

    //////////////////////////////////////////////////////////////////////////
    // ImGui windows etc.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool openFileDialog = false;
    bool reloadMeshConfig = false;
    bool saveMeshConfig = false;
    
    ImGui::SetMouseCursor( hideCursorTimer <= 5.0f ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_None );

    if ( ImGui::IsKeyPressed( GLFW_KEY_F, false ) )
    {
      gCameraTarget = ( gModel.mAABBMin + gModel.mAABBMax ) / 2.0f;
      gCameraDistance = glm::length( gCameraTarget - gModel.mAABBMin ) * 4.0f;
      gCameraYaw = glm::pi<float>() / 4.0f;
      gCameraPitch = 0.25f;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_F11, false ) )
    {
      showImGui = !showImGui;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_W, false ) )
    {
      edgedFaces = !edgedFaces;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_C, false ) )
    {
      automaticCamera = !automaticCamera;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_O, false ) && ( ImGui::IsKeyDown( GLFW_KEY_LEFT_CONTROL ) || ImGui::IsKeyDown( GLFW_KEY_RIGHT_CONTROL ) ) )
    {
      openFileDialog = true;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_S, false ) && ( ImGui::IsKeyDown( GLFW_KEY_LEFT_CONTROL ) || ImGui::IsKeyDown( GLFW_KEY_RIGHT_CONTROL ) ) )
    {
      saveMeshConfig = true;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_L, false ) && ( ImGui::IsKeyDown( GLFW_KEY_LEFT_CONTROL ) || ImGui::IsKeyDown( GLFW_KEY_RIGHT_CONTROL ) ) )
    {
      reloadMeshConfig = true;
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_ENTER, false ) && ( ImGui::IsKeyDown( GLFW_KEY_LEFT_ALT ) || ImGui::IsKeyDown( GLFW_KEY_RIGHT_ALT ) ) )
    {
      Renderer::SwitchFullscreen( Renderer::eMode == RENDERER_WINDOWMODE_WINDOWED ? RENDERER_WINDOWMODE_FULLSCREEN : RENDERER_WINDOWMODE_WINDOWED );
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_PAGE_UP, false ) )
    {
      const int shaderCount = gOptions.get<jsonxx::Array>( "shaders" ).size();
      for ( int i = 0; i < shaderCount; i++ )
      {
        const jsonxx::Object & shaderConfig = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( i );
        if ( &shaderConfig == gCurrentShaderConfig )
        {
          const jsonxx::Object & newShaderConfig = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( ( i - 1 + shaderCount ) % shaderCount );
          LoadShaderConfig( &newShaderConfig );
          gModel.RebindVertexArray( gCurrentShader );
          break;
        }
      }
    }
    if ( ImGui::IsKeyPressed( GLFW_KEY_PAGE_DOWN, false ) )
    {
      const int shaderCount = gOptions.get<jsonxx::Array>( "shaders" ).size();
      for ( int i = 0; i < shaderCount; i++ )
      {
        const jsonxx::Object & shaderConfig = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( i );
        if ( &shaderConfig == gCurrentShaderConfig )
        {
          const jsonxx::Object & newShaderConfig = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( ( i + 1 + shaderCount ) % shaderCount );
          LoadShaderConfig( &newShaderConfig );
          gModel.RebindVertexArray( gCurrentShader );
          break;
        }
      }
    }

    if ( showImGui )
    {
      if ( ImGui::BeginMainMenuBar() )
      {
        if ( ImGui::BeginMenu( "File" ) )
        {
          if ( ImGui::MenuItem( "Open model...", "Ctrl-O" ) )
          {
            openFileDialog = true;
          }
          ImGui::Separator();
          if ( ImGui::MenuItem( "Reload model config", "Ctrl-L" ) )
          {
            reloadMeshConfig = true;
          }
          if ( ImGui::MenuItem( "Save model config", "Ctrl-S" ) )
          {
            saveMeshConfig = true;
          }
          ImGui::Separator();
          if ( ImGui::MenuItem( "Exit", "Alt-F4" ) )
          {
            appWantsToQuit = true;
          }
          ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Model" ) )
        {
          ImGui::MenuItem( "Show model info", NULL, &showModelInfo );
          ImGui::Separator();

          bool xyzSpace = !xzySpace;
          if ( ImGui::MenuItem( "XYZ space", NULL, &xyzSpace ) )
          {
            xzySpace = !xzySpace;
          }
          ImGui::MenuItem( "XZY space", NULL, &xzySpace );
          ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "View" ) )
        {
          if ( ImGui::MenuItem( "Windowed", "Alt-Enter", Renderer::eMode == RENDERER_WINDOWMODE_WINDOWED ) )
          {
            Renderer::SwitchFullscreen( RENDERER_WINDOWMODE_WINDOWED );
          }
          if ( ImGui::MenuItem( "Fullscreen", "Alt-Enter", Renderer::eMode == RENDERER_WINDOWMODE_FULLSCREEN ) )
          {
            Renderer::SwitchFullscreen( RENDERER_WINDOWMODE_FULLSCREEN );
          }
          ImGui::Separator();

          ImGui::MenuItem( "Wireframe / Edged faces", "W", &edgedFaces );
          ImGui::MenuItem( "Show menu", "F11", &showImGui );
          ImGui::Separator();

          ImGui::MenuItem( "Enable idle camera", "C", &automaticCamera );
          if ( ImGui::MenuItem( "Re-center camera", "F" ) )
          {
            gCameraTarget = ( gModel.mAABBMin + gModel.mAABBMax ) / 2.0f;
            gCameraDistance = glm::length( gCameraTarget - gModel.mAABBMin ) * 4.0f;
            gCameraYaw = glm::pi<float>() / 4.0f;
            gCameraPitch = 0.25f;
          }
          ImGui::Separator();

          ImGui::ColorEdit4( "Background", (float *) &gClearColor, ImGuiColorEditFlags_AlphaPreviewHalf );
          ImGui::Separator();

          ImGui::DragFloat( "Environment exposure", &exposure, 0.01f, 0.1f, 4.0f );
          ImGui::DragFloat( "Sky blur", &gSkysphereBlur, 0.01f, 0.0f, 1.0f );
          ImGui::DragFloat( "Sky opacity", &gSkysphereOpacity, 0.02f, 0.0f, 1.0f );
#ifdef _DEBUG
          ImGui::Separator();
          ImGui::DragFloat( "Camera Yaw", &gCameraYaw, 0.01f );
          ImGui::DragFloat( "Camera Pitch", &gCameraPitch, 0.01f );
          ImGui::DragFloat3( "Camera Target", (float *) &gCameraTarget );
          ImGui::DragFloat( "Light Yaw", &gLightYaw, 0.01f );
          ImGui::DragFloat( "Light Pitch", &gLightPitch, 0.01f );
#endif
          ImGui::EndMenu();
        }
        if ( ImGui::BeginMenu( "Shaders" ) )
        {
          for ( int i = 0; i < gOptions.get<jsonxx::Array>( "shaders" ).size(); i++ )
          {
            const jsonxx::Object & shaderConfig = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( i );
            const std::string & name = gOptions.get<jsonxx::Array>( "shaders" ).get<jsonxx::Object>( i ).get<jsonxx::String>( "name" );

            bool selected = &shaderConfig == gCurrentShaderConfig;
            if ( ImGui::MenuItem( name.c_str(), NULL, &selected ) )
            {
              LoadShaderConfig( &shaderConfig );
              gModel.RebindVertexArray( gCurrentShader );
            }
          }
          if ( gCurrentShaderConfig && gCurrentShaderConfig->get<jsonxx::Boolean>( "showSkybox" ) )
          {
            ImGui::Separator();
            for ( int i = 0; i < gOptions.get<jsonxx::Array>( "skyImages" ).size(); i++ )
            {
              const auto & images = gOptions.get<jsonxx::Array>( "skyImages" ).get<jsonxx::Object>( i );
              const std::string & filename = images.get<jsonxx::String>( "reflection" );

              bool selected = ( gCurrentSkyImage.reflection && gCurrentSkyImage.reflection->mFilename == filename );
              if ( ImGui::MenuItem( filename.c_str(), NULL, &selected ) )
              {
                LoadSkyImageConfig( images );
                gLightYaw = gCurrentSkyImage.sunYaw;
                gLightPitch = gCurrentSkyImage.sunPitch;
              }
            }
          }
          ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
      }
    }

    if ( openFileDialog )
    {
      ImGui::OpenPopup( "Open model" );
    }
    if ( reloadMeshConfig )
    {
      char meshConfigPath[ 512 ];
      snprintf( meshConfigPath, 512, "%s.foxocfg", gMeshPath.c_str() );
      LoadMeshConfig( meshConfigPath );
    }
    if ( saveMeshConfig )
    {
      char meshConfigPath[ 512 ];
      snprintf( meshConfigPath, 512, "%s.foxocfg", gMeshPath.c_str() );
      SaveMeshConfig( meshConfigPath );
    }

    if ( file_dialog.showFileDialog( "Open model", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2( 700, 310 ), supportedExtensions.c_str() ) )
    {
      LoadMesh( file_dialog.selected_path.c_str() );
    }

    if ( showModelInfo )
    {
      ImGui::Begin( "Model info", &showModelInfo );
      ImGui::BeginTabBar( "model" );
      if ( ImGui::BeginTabItem( "Summary" ) )
      {
        int triCount = 0;
        for ( std::map<int, Geometry::Mesh>::iterator it = gModel.mMeshes.begin(); it != gModel.mMeshes.end(); it++ )
        {
          triCount += it->second.mTriangleCount;
        }

        ImGui::Text( "Triangle count: %d", triCount );
        ImGui::Text( "Mesh count: %d", gModel.mMeshes.size() );

        ImGui::EndTabItem();
      }
      if ( ImGui::BeginTabItem( "Node tree" ) )
      {
        ShowNodeInImGui( -1 );

        ImGui::EndTabItem();
      }
      if ( ImGui::BeginTabItem( "Textures / Materials" ) )
      {
        ImGui::Text( "Material count: %d", gModel.mMaterials.size() );

        for ( std::map<int, Geometry::Material>::iterator it = gModel.mMaterials.begin(); it != gModel.mMaterials.end(); it++ )
        {
          if ( ImGui::CollapsingHeader( it->second.mName.c_str() ) )
          {
            ImGui::Indent();
            ImGui::Text( "Specular shininess: %g", it->second.mSpecularShininess );
            if ( ImGui::BeginTabBar( it->second.mName.c_str() ) )
            {
              ShowColorMapInImGui( "Ambient", it->second.mColorMapAmbient );
              ShowColorMapInImGui( "Diffuse", it->second.mColorMapDiffuse );
              ShowColorMapInImGui( "Normals", it->second.mColorMapNormals );
              ShowColorMapInImGui( "Specular", it->second.mColorMapSpecular );
              ShowColorMapInImGui( "Albedo", it->second.mColorMapAlbedo );
              ShowColorMapInImGui( "Metallic", it->second.mColorMapMetallic );
              ShowColorMapInImGui( "Roughness", it->second.mColorMapRoughness );
              ShowColorMapInImGui( "AO", it->second.mColorMapAO );
              ShowColorMapInImGui( "Emissive", it->second.mColorMapEmissive );
              ImGui::EndTabBar();
            }
            ImGui::Unindent();
          }
        }

        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
      ImGui::End();
    }

    bool showHelpText = ( gModel.mNodes.size() == 0 );
    if ( showHelpText )
    {
      ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f ), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
      ImGui::SetNextWindowSize( ImVec2( 380.0f, 190.0f ) );
      ImGui::SetNextWindowBgAlpha( 0.5f );

      ImGui::Begin( "HelpText", &showHelpText, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove );
      ImGui::TextColored( ImColor( 255, 127, 0 ), "Welcome to FOXOTRON!" );
      ImGui::NewLine();
      ImGui::Text( "To load a model:" );
      ImGui::BulletText( "Use File > Open in the menu in the top left" );
      ImGui::BulletText( "Or drag'n'drop a model here" );
      ImGui::NewLine();
      ImGui::Text( "Once a model is loaded:" );
      ImGui::BulletText( "Left mouse button to rotate" );
      ImGui::BulletText( "Right mouse button to move light / rotate sky" );
      ImGui::BulletText( "Middle mouse button to pan camera" );
      ImGui::End();
    }

    //////////////////////////////////////////////////////////////////////////
    // Drag'n'drop

    for ( int i = 0; i < Renderer::dropEventBufferCount; i++ )
    {
      std::string & path = Renderer::dropEventBuffer[ i ];
      LoadMesh( path.c_str() );
    }
    Renderer::dropEventBufferCount = 0;

    //////////////////////////////////////////////////////////////////////////
    // Mouse rotation

    if ( !io.WantCaptureMouse )
    {
      ImVec2 mouseEvent = ImGui::GetMousePos();

      if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
      {
        rotatingCamera = true;
        automaticCamera = false;
        mouseClickPosX = mouseEvent.x;
        mouseClickPosY = mouseEvent.y;
      }
      if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
      {
        rotatingCamera = false;
      }

      if ( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
      {
        movingLight = true;
        mouseClickPosX = mouseEvent.x;
        mouseClickPosY = mouseEvent.y;
      }
      if ( !ImGui::IsMouseDown( ImGuiMouseButton_Right ) )
      {
        movingLight = false;
      }

      if ( ImGui::IsMouseClicked( ImGuiMouseButton_Middle ) )
      {
        movingCamera = true;
        mouseClickPosX = mouseEvent.x;
        mouseClickPosY = mouseEvent.y;
      }
      if ( !ImGui::IsMouseDown( ImGuiMouseButton_Middle ) )
      {
        movingCamera = false;
      }

      const float panSpeed = 1000.0f;
      const float rotationSpeed = 130.0f;
      if ( rotatingCamera )
      {
        gCameraYaw -= ( mouseEvent.x - mouseClickPosX ) / rotationSpeed;
        gCameraPitch += ( mouseEvent.y - mouseClickPosY ) / rotationSpeed;

        // Clamp to avoid gimbal lock
        gCameraPitch = std::min( gCameraPitch, 1.5f );
        gCameraPitch = std::max( gCameraPitch, -1.5f );

        mouseClickPosX = mouseEvent.x;
        mouseClickPosY = mouseEvent.y;
      }
      if ( movingLight )
      {
        gLightYaw += ( mouseEvent.x - mouseClickPosX ) / rotationSpeed;
        gLightPitch -= ( mouseEvent.y - mouseClickPosY ) / rotationSpeed;

        // Clamp to avoid gimbal lock
        gLightPitch = std::min( gLightPitch, 1.5f );
        gLightPitch = std::max( gLightPitch, -1.5f );

        mouseClickPosX = mouseEvent.x;
        mouseClickPosY = mouseEvent.y;
      }
      if ( movingCamera )
      {
        const float moveX = ( mouseEvent.x - mouseClickPosX ) / panSpeed;
        const float moveY = ( mouseEvent.y - mouseClickPosY ) / panSpeed;

        glm::vec3 newBaseZ( 0.0f, 0.0f, -1.0f );
        newBaseZ = glm::rotateX( newBaseZ, gCameraPitch );
        newBaseZ = glm::rotateY( newBaseZ, gCameraYaw );
        newBaseZ = -newBaseZ;

        glm::vec3 up( 0.0f, 1.0f, 0.0f );
        glm::vec3 newBaseX = glm::cross( up, newBaseZ );
        glm::vec3 newBaseY = glm::cross( newBaseZ, newBaseX );

        gCameraTarget += newBaseX * moveX * gCameraDistance;
        gCameraTarget += newBaseY * moveY * gCameraDistance;

        mouseClickPosX = mouseEvent.x;
        mouseClickPosY = mouseEvent.y;
      }
      if ( io.MouseWheel != 0.0f )
      {
        const float aspect = 1.1f;
        gCameraDistance *= io.MouseWheel < 0 ? aspect : 1 / aspect;
      }

      if ( io.MouseDelta.x != 0.0f && io.MouseDelta.y != 0.0f )
      {
        hideCursorTimer = 0.0f;
      }
    }
    else
    {
      hideCursorTimer = 0.0f;
    }
    hideCursorTimer += io.DeltaTime;

    ImGui::Render();

    //////////////////////////////////////////////////////////////////////////
    // Camera and lights

    if ( automaticCamera )
    {
      gCameraYaw += io.DeltaTime * 0.3f;
    }

    //////////////////////////////////////////////////////////////////////////
    // Skysphere render

    glm::vec3 cameraPosition( 0.0f, 0.0f, -1.0f );
    cameraPosition = glm::rotateX( cameraPosition, gCameraPitch );
    cameraPosition = glm::rotateY( cameraPosition, gCameraYaw );

    static glm::mat4x4 worldRootXYZ( 1.0f );
    if ( gCurrentShaderConfig->get<jsonxx::Boolean>( "showSkybox" ) )
    {
      float verticalFovInRadian = 0.5f;
      projectionMatrix = glm::perspective( verticalFovInRadian, settings.mWidth / (float) settings.mHeight, 0.001f, 2.0f );
      skysphereShader->SetConstant( "mat_projection", projectionMatrix );

      viewMatrix = glm::lookAtRH( cameraPosition * 0.15f, glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
      skysphereShader->SetConstant( "mat_view", viewMatrix );

      skysphereShader->SetConstant( "has_tex_skysphere", gCurrentSkyImage.reflection != NULL );
      skysphereShader->SetConstant( "has_tex_skyenv", gCurrentSkyImage.env != NULL );

      if ( gCurrentSkyImage.reflection )
      {
        const float mipCount = floor( log2( gCurrentSkyImage.reflection->mHeight ) );
        skysphereShader->SetTexture( "tex_skysphere", gCurrentSkyImage.reflection );
        skysphereShader->SetConstant( "skysphere_mip_count", mipCount );
      }

      if ( gCurrentSkyImage.env )
      {
        skysphereShader->SetTexture( "tex_skyenv", gCurrentSkyImage.env );
      }

      skysphereShader->SetConstant( "background_color", gClearColor );
      skysphereShader->SetConstant( "skysphere_blur", gSkysphereBlur );
      skysphereShader->SetConstant( "skysphere_opacity", gSkysphereOpacity );
      skysphereShader->SetConstant( "skysphere_rotation", gLightYaw - gCurrentSkyImage.sunYaw  );
      skysphereShader->SetConstant( "exposure", exposure );
      skysphereShader->SetConstant( "frame_count", frameCount );

      skysphere.Render( worldRootXYZ, skysphereShader );

      glClear( GL_DEPTH_BUFFER_BIT );
    }

    //////////////////////////////////////////////////////////////////////////
    // Mesh render

    const float verticalFovInRadian = 0.5f;
    const float nearPlane = std::max( gModel.mModelDiagonal / 10000.0f, gCameraDistance / 1000.0f );
    const float farPlane = std::max( gModel.mModelDiagonal, gCameraDistance + gModel.mModelDiagonal );
    projectionMatrix = glm::perspective( verticalFovInRadian, settings.mWidth / (float) settings.mHeight, nearPlane, farPlane );
    gCurrentShader->SetConstant( "mat_projection", projectionMatrix );

    cameraPosition *= gCameraDistance;
    gCurrentShader->SetConstant( "camera_position", cameraPosition );

    glm::vec3 lightDirection( 0.0f, 0.0f, 1.0f );
    lightDirection = glm::rotateX( lightDirection, gLightPitch );
    lightDirection = glm::rotateY( lightDirection, gLightYaw );

    glm::vec3 fillLightDirection( 0.0f, 0.0f, 1.0f );
    fillLightDirection = glm::rotateX( fillLightDirection, gLightPitch - 0.4f );
    fillLightDirection = glm::rotateY( fillLightDirection, gLightYaw + 0.8f );

    gCurrentShader->SetConstant( "lights[0].direction", lightDirection );
    gCurrentShader->SetConstant( "lights[0].color", gCurrentSkyImage.sunColor );
    gCurrentShader->SetConstant( "lights[1].direction", fillLightDirection );
    gCurrentShader->SetConstant( "lights[1].color", glm::vec3( 0.5f ) );
    gCurrentShader->SetConstant( "lights[2].direction", -fillLightDirection );
    gCurrentShader->SetConstant( "lights[2].color", glm::vec3( 0.25f ) );

    gCurrentShader->SetConstant( "skysphere_rotation", gLightYaw - gCurrentSkyImage.sunYaw );

    viewMatrix = glm::lookAtRH( cameraPosition + gCameraTarget, gCameraTarget, glm::vec3( 0.0f, 1.0f, 0.0f ) );
    gCurrentShader->SetConstant( "mat_view", viewMatrix );
    gCurrentShader->SetConstant( "mat_view_inverse", glm::inverse( viewMatrix ) );

    gCurrentShader->SetConstant( "has_tex_skysphere", gCurrentSkyImage.reflection != NULL );
    gCurrentShader->SetConstant( "has_tex_skyenv", gCurrentSkyImage.env != NULL );
    if ( gCurrentSkyImage.reflection )
    {
      float mipCount = floor( log2( gCurrentSkyImage.reflection->mHeight ) );
      gCurrentShader->SetTexture( "tex_skysphere", gCurrentSkyImage.reflection );
      gCurrentShader->SetConstant( "skysphere_mip_count", mipCount );
    }
    if ( gCurrentSkyImage.env )
    {
      gCurrentShader->SetTexture( "tex_skyenv", gCurrentSkyImage.env );
    }
    gCurrentShader->SetTexture( "tex_brdf_lut", gBrdfLookupTable );
    gCurrentShader->SetConstant( "exposure", exposure );
    gCurrentShader->SetConstant( "frame_count", frameCount );

    //////////////////////////////////////////////////////////////////////////
    // Mesh render

    gModel.Render( xzySpace ? xzyMatrix : worldRootXYZ, gCurrentShader );

    if ( edgedFaces )
    {
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      glDepthFunc( GL_LEQUAL );

      gCurrentShader->SetConstant( "exposure", 100.0f );
      gModel.Render( xzySpace ? xzyMatrix : worldRootXYZ, gCurrentShader );

      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      glDepthFunc( GL_LESS );
    }

    //////////////////////////////////////////////////////////////////////////
    // End frame
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    Renderer::EndFrame();
    frameCount++;
  }

  //////////////////////////////////////////////////////////////////////////
  // Cleanup

  if ( gCurrentShader )
  {
    Renderer::ReleaseShader( gCurrentShader );
    delete gCurrentShader;
  }
  if ( skysphereShader )
  {
    Renderer::ReleaseShader( skysphereShader );
    delete skysphereShader;
  }
  if ( gCurrentSkyImage.reflection )
  {
    Renderer::ReleaseTexture( gCurrentSkyImage.reflection );
    gCurrentSkyImage.reflection = NULL;
  }
  if ( gCurrentSkyImage.env )
  {
    Renderer::ReleaseTexture( gCurrentSkyImage.env );
    gCurrentSkyImage.env = NULL;
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  gModel.UnloadMesh();

  Renderer::Close();

  return 0;
}