#include <stdio.h>
#include <algorithm>

#include "Geometry.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "ImGuiFileBrowser.h"

#include "ext.hpp"
#include "gtx/rotate_vector.hpp"

struct ShaderConfig
{
  const char * mName;
  const char * mVertexShaderPath;
  const char * mFragmentShaderPath;
  bool mSkysphereVisible;
} gShaders[] = {
  { "Physically Based",              "Shaders/pbr.vs",                 "Shaders/pbr.fs"            ,     false },
  { "Basic SpecGloss",               "Shaders/basic_specgloss.vs",     "Shaders/basic_specgloss.fs",     false },
  { NULL, NULL, NULL, },
};

const char * skyboxFilenames[] =
{
  "Skyboxes/Barce_Rooftop_C_3k.hdr",
  "Skyboxes/GCanyon_C_YumaPoint_3k.hdr",
  "Skyboxes/Ridgecrest_Road_Ref.hdr",
  "Skyboxes/Road_to_MonumentValley_Ref.hdr",
  NULL
};

Renderer::Shader * LoadShader( const char * vsPath, const char * fsPath )
{
  char vertexShader[ 16 * 1024 ] = { 0 };
  FILE * fileVS = fopen( vsPath, "rb" );
  if ( !fileVS )
  {
    printf( "Vertex shader load failed: '%s'\n", vsPath );
    return false;
  }
  fread( vertexShader, 1, 16 * 1024, fileVS );
  fclose( fileVS );

  char fragmentShader[ 16 * 1024 ] = { 0 };
  FILE * fileFS = fopen( fsPath, "rb" );
  if ( !fileFS )
  {
    printf( "Fragment shader load failed: '%s'\n", fsPath );
    return false;
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

ShaderConfig * gCurrentShaderConfig = NULL;
Renderer::Shader * gCurrentShader = NULL;
bool LoadShaderConfig( ShaderConfig * shader )
{
  Renderer::Shader * newShader = LoadShader( shader->mVertexShaderPath, shader->mFragmentShaderPath );
  if ( !newShader )
  {
    return false;
  }

  gCurrentShaderConfig = shader;

  if ( gCurrentShader )
  {
    Renderer::ReleaseShader( gCurrentShader );
    delete gCurrentShader;
  }
  gCurrentShader = newShader;

  return true;
}

glm::vec3 gCameraTarget( 0.0f, 0.0f, 0.0f );
float gCameraDistance = 500.0f;
Geometry gModel;

bool LoadMesh( const char * path )
{
  if ( !gModel.LoadMesh( path ) )
  {
    return false;
  }

  gModel.RebindVertexArray( gCurrentShader );

  gCameraTarget = ( gModel.mAABBMin + gModel.mAABBMax ) / 2.0f;
  gCameraDistance = glm::length( gCameraTarget - gModel.mAABBMin ) * 4.0f;

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
      }

      ShowNodeInImGui( it->second.mID );
      ImGui::Unindent();
    }
  }
}

void ShowMaterialInImGui( const char * _channel, Renderer::Texture * _texture )
{
  if ( !_texture )
  {
    return;
  }
  if ( ImGui::BeginTabItem( _channel ) )
  {
    ImGui::Text( "Texture: %s", _texture->mFilename.c_str() );
    ImGui::Text( "Dimensions: %d x %d", _texture->mWidth, _texture->mHeight );
    ImGui::Image( (ImTextureID) _texture->mGLTextureID, ImVec2( 512.0f, 512.0f ) );
    ImGui::EndTabItem();
  }
}

Renderer::Texture * gSkySphereTexture = NULL;

int main( int argc, const char * argv[] )
{
  //////////////////////////////////////////////////////////////////////////
  // Init renderer
  RENDERER_SETTINGS settings;
  settings.bVsync = false;
  settings.nWidth = 1280;
  settings.nHeight = 720;
  settings.windowMode = RENDERER_WINDOWMODE_WINDOWED;
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
  if ( !LoadShaderConfig( &gShaders[ 0 ] ) )
  {
    return -4;
  }

  if ( argc >= 2 )
  {
    LoadMesh( argv[ 1 ] );
  }

  //////////////////////////////////////////////////////////////////////////
  // Mainloop
  bool appWantsToQuit = false;
  bool automaticCamera = false;
  glm::mat4x4 viewMatrix;
  glm::mat4x4 projectionMatrix;
  bool movingCamera = false;
  bool movingLight = false;
  float cameraYaw = 0.0f;
  float cameraPitch = 0.0f;
  float lightYaw = 0.0f;
  float lightPitch = 0.0f;
  float mouseClickPosX = 0.0f;
  float mouseClickPosY = 0.0f;
  glm::vec4 clearColor( 0.08f, 0.18f, 0.18f, 1.0f );
  std::string supportedExtensions = Geometry::GetSupportedExtensions();
  float skysphereOpacity = 1.0f;
  float skysphereBlur = 0.75f;

  bool xzySpace = false;
  const glm::mat4x4 xzyMatrix(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f,-1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f );

  gSkySphereTexture = Renderer::CreateRGBA8TextureFromFile( skyboxFilenames[ 0 ] );

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
    Renderer::StartFrame( clearColor );

    //////////////////////////////////////////////////////////////////////////
    // ImGui windows etc.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    bool openFileDialog = false;
    static bool showModelInfo = false;
    if ( ImGui::BeginMainMenuBar() )
    {
      if ( ImGui::BeginMenu( "File" ) )
      {
        if ( ImGui::MenuItem( "Open model..." ) )
        {
          openFileDialog = true;
        }
        ImGui::Separator();
        if ( ImGui::MenuItem( "Exit" ) )
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
        ImGui::MenuItem( "Enable idle camera", NULL, &automaticCamera );
        ImGui::ColorEdit4( "Background", (float *) &clearColor, ImGuiColorEditFlags_AlphaPreviewHalf );
#ifdef _DEBUG
        ImGui::Separator();
        ImGui::DragFloat3( "Camera Target", (float *) &gCameraTarget );
#endif
        ImGui::Separator();
        ImGui::DragFloat( "Sky blur", &skysphereBlur, 0.1f, 0.0f, 9.0f );
        ImGui::DragFloat( "Sky opacity", &skysphereOpacity, 0.02f, 0.0f, 1.0f );
        ImGui::EndMenu();
      }
      if ( ImGui::BeginMenu( "Shaders" ) )
      {
        for ( int i = 0; gShaders[ i ].mName; i++ )
        {
          bool selected = &gShaders[ i ] == gCurrentShaderConfig;
          if ( ImGui::MenuItem( gShaders[ i ].mName, NULL, &selected ) )
          {
            LoadShaderConfig( &gShaders[ i ] );
          }
          gModel.RebindVertexArray( gCurrentShader );
        }
        if ( gCurrentShaderConfig && gCurrentShaderConfig->mSkysphereVisible )
        {
          ImGui::Separator();
          for ( int i = 0; skyboxFilenames[ i ]; i++ )
          {
            bool selected = ( gSkySphereTexture && gSkySphereTexture->mFilename == skyboxFilenames[ i ] );
            if ( ImGui::MenuItem( skyboxFilenames[ i ], NULL, &selected ) )
            {
              if ( gSkySphereTexture )
              {
                Renderer::ReleaseTexture( gSkySphereTexture );
                delete gSkySphereTexture;

                gSkySphereTexture = NULL;
              }
              gSkySphereTexture = Renderer::CreateRGBA8TextureFromFile( skyboxFilenames[ i ] );
            }
          }
        }
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    if ( openFileDialog )
    {
      ImGui::OpenPopup( "Open model" );
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
            ImGui::ColorEdit4( "Ambient color", (float *) &it->second.mColorAmbient, ImGuiColorEditFlags_AlphaPreviewHalf );
            ImGui::ColorEdit4( "Diffuse color", (float *) &it->second.mColorDiffuse, ImGuiColorEditFlags_AlphaPreviewHalf );
            ImGui::ColorEdit4( "Specular color", (float *) &it->second.mColorSpecular, ImGuiColorEditFlags_AlphaPreviewHalf );
            if ( ImGui::BeginTabBar( it->second.mName.c_str() ) )
            {
              ShowMaterialInImGui( "Diffuse", it->second.mTextureDiffuse );
              ShowMaterialInImGui( "Normals", it->second.mTextureNormals );
              ShowMaterialInImGui( "Specular", it->second.mTextureSpecular );
              ShowMaterialInImGui( "Albedo", it->second.mTextureAlbedo );
              ShowMaterialInImGui( "Metallic", it->second.mTextureMetallic );
              ShowMaterialInImGui( "Roughness", it->second.mTextureRoughness );
              ShowMaterialInImGui( "AO", it->second.mTextureAO );
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

    ImGui::Render();

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
      for ( int i = 0; i < Renderer::mouseEventBufferCount; i++ )
      {
        Renderer::MouseEvent & mouseEvent = Renderer::mouseEventBuffer[ i ];
        switch ( mouseEvent.eventType )
        {
          case Renderer::MOUSEEVENTTYPE_MOVE:
            {
              const float rotationSpeed = 130.0f;
              if ( movingCamera )
              {
                cameraYaw -= ( mouseEvent.x - mouseClickPosX ) / rotationSpeed;
                cameraPitch += ( mouseEvent.y - mouseClickPosY ) / rotationSpeed;

                // Clamp to avoid gimbal lock
                cameraPitch = std::min( cameraPitch, 1.5f );
                cameraPitch = std::max( cameraPitch, -1.5f );
              }
              if ( movingLight )
              {
                lightYaw -= ( mouseEvent.x - mouseClickPosX ) / rotationSpeed;
                lightPitch -= ( mouseEvent.y - mouseClickPosY ) / rotationSpeed;

                // Clamp to avoid gimbal lock
                lightPitch = std::min( lightPitch, 1.5f );
                lightPitch = std::max( lightPitch, -1.5f );
              }
              mouseClickPosX = mouseEvent.x;
              mouseClickPosY = mouseEvent.y;
            }
            break;
          case Renderer::MOUSEEVENTTYPE_DOWN:
            {
              if ( mouseEvent.button == Renderer::MOUSEBUTTON_LEFT )
              {
                movingCamera = true;
                automaticCamera = false;
              }
              else if ( mouseEvent.button == Renderer::MOUSEBUTTON_RIGHT )
              {
                movingLight = true;
              }
              mouseClickPosX = mouseEvent.x;
              mouseClickPosY = mouseEvent.y;
            }
            break;
          case Renderer::MOUSEEVENTTYPE_UP:
            {
              if ( mouseEvent.button == Renderer::MOUSEBUTTON_LEFT )
              {
                movingCamera = false;
              }
              else if ( mouseEvent.button == Renderer::MOUSEBUTTON_RIGHT )
              {
                movingLight = false;
              }
            }
            break;
          case Renderer::MOUSEEVENTTYPE_SCROLL:
            {
              const float aspect = 1.1f;
              gCameraDistance *= mouseEvent.y < 0 ? aspect : 1 / aspect;
            }
            break;
        }
      }
    }
    Renderer::mouseEventBufferCount = 0;

    //////////////////////////////////////////////////////////////////////////
    // Camera and lights

    if ( automaticCamera )
    {
      cameraYaw += io.DeltaTime * 0.3f;
    }

    //////////////////////////////////////////////////////////////////////////
    // Skysphere render

    glm::vec3 cameraPosition( 0.0f, 0.0f, -1.0f );
    cameraPosition = glm::rotateX( cameraPosition, cameraPitch );
    cameraPosition = glm::rotateY( cameraPosition, cameraYaw );

    static glm::mat4x4 worldRootXYZ( 1.0f );
    if ( gCurrentShaderConfig->mSkysphereVisible )
    {
      float verticalFovInRadian = 0.5f;
      projectionMatrix = glm::perspective( verticalFovInRadian, settings.nWidth / (float) settings.nHeight, 0.001f, 2.0f );
      skysphereShader->SetConstant( "mat_projection", projectionMatrix );

      viewMatrix = glm::lookAtRH( cameraPosition * 0.15f, glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
      skysphereShader->SetConstant( "mat_view", viewMatrix );

      skysphereShader->SetConstant( "has_tex_skysphere", gSkySphereTexture != NULL );
      if ( gSkySphereTexture )
      {
        skysphereShader->SetTexture( "tex_skysphere", gSkySphereTexture );
      }

      skysphereShader->SetConstant( "skysphere_blur", skysphereBlur );
      skysphereShader->SetConstant( "skysphere_opacity", skysphereOpacity );
      skysphereShader->SetConstant( "skysphere_rotation", lightYaw );

      skysphere.Render( worldRootXYZ, skysphereShader );
    }

    //////////////////////////////////////////////////////////////////////////
    // Mesh render

    float verticalFovInRadian = 0.5f;
    projectionMatrix = glm::perspective( verticalFovInRadian, settings.nWidth / (float) settings.nHeight, gCameraDistance / 1000.0f, gCameraDistance * 2.0f );
    gCurrentShader->SetConstant( "mat_projection", projectionMatrix );

    cameraPosition *= gCameraDistance;
    gCurrentShader->SetConstant( "camera_position", cameraPosition );

    glm::vec3 lightDirection( 0.0f, 0.0f, 1.0f );
    lightDirection = glm::rotateX( lightDirection, lightPitch );
    lightDirection = glm::rotateY( lightDirection, lightYaw );

    glm::vec3 fillLightDirection( 0.0f, 0.0f, 1.0f );
    fillLightDirection = glm::rotateX( fillLightDirection, lightPitch - 0.4f );
    fillLightDirection = glm::rotateY( fillLightDirection, lightYaw + 0.8f );

    gCurrentShader->SetConstant( "lights[0].direction", lightDirection );
    gCurrentShader->SetConstant( "lights[0].color", glm::vec3( 1.0f ) );
    gCurrentShader->SetConstant( "lights[1].direction", fillLightDirection );
    gCurrentShader->SetConstant( "lights[1].color", glm::vec3( 0.5f ) );
    gCurrentShader->SetConstant( "lights[2].direction", -fillLightDirection );
    gCurrentShader->SetConstant( "lights[2].color", glm::vec3( 0.25f ) );

    gCurrentShader->SetConstant( "skysphere_rotation", lightYaw );

    viewMatrix = glm::lookAtRH( cameraPosition + gCameraTarget, gCameraTarget, glm::vec3( 0.0f, 1.0f, 0.0f ) );
    gCurrentShader->SetConstant( "mat_view", viewMatrix );

    gCurrentShader->SetConstant( "has_tex_skysphere", gSkySphereTexture != NULL );
    if ( gSkySphereTexture )
    {
      gCurrentShader->SetTexture( "tex_skysphere", gSkySphereTexture );
    }

    //////////////////////////////////////////////////////////////////////////
    // Mesh render

    gModel.Render( xzySpace ? xzyMatrix : worldRootXYZ, gCurrentShader );

    //////////////////////////////////////////////////////////////////////////
    // End frame
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    Renderer::EndFrame();
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
  if ( gSkySphereTexture )
  {
    Renderer::ReleaseTexture( gSkySphereTexture );
    delete gSkySphereTexture;
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  gModel.UnloadMesh();

  Renderer::Close();

  return 0;
}