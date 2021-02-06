#include <stdio.h>

#include "Renderer.h"
#include "Geometry.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "ext.hpp"

struct Shader
{
  const char * name;
  const char * vertexShaderPath;
  const char * fragmentShaderPath;
} shaders[] = {
  { "Basic SpecGloss", "Shaders/basic_specgloss.vs", "Shaders/basic_specgloss.fs" },
  { NULL, NULL, NULL, },
};

Shader * current_shader = NULL;
bool load_shader( Shader * shader )
{
  char vertexShader[ 16 * 1024 ] = { 0 };
  FILE * fileVS = fopen( shader->vertexShaderPath, "rb" );
  if ( fileVS )
  {
    fread( vertexShader, 1, 16 * 1024, fileVS );
    fclose( fileVS );
  }

  char fragmentShader[ 16 * 1024 ] = { 0 };
  FILE * fileFS = fopen( shader->fragmentShaderPath, "rb" );
  if ( fileFS )
  {
    fread( fragmentShader, 1, 16 * 1024, fileFS );
    fclose( fileFS );
  }

  char error[ 4096 ];
  if ( !Renderer::ReloadShaders( vertexShader, (int) strlen( vertexShader ), fragmentShader, (int) strlen( fragmentShader ), error, 4096 ) )
  {
    printf( "Shader load failed: %s\n", error );
    return false;
  }

  current_shader = shader;

  return true;
}

int main( int argc, const char * argv[] )
{
  if ( argc < 2 )
  {
    printf( "needs cmdline for now" );
    return -2;
  }

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

  if ( !Geometry::LoadMesh( argv[ 1 ] ) )
  {
    return -3;
  }

  if ( !load_shader( &shaders[0] ) )
  {
    return -4;
  }

  //////////////////////////////////////////////////////////////////////////
  // Mainloop
  bool appWantsToQuit = false;
  bool automaticCamera = false;
  glm::mat4x4 viewMatrix;
  glm::mat4x4 projectionMatrix;
  glm::vec3 cameraPosition( 500.0f, 500.0f, -500.0f );

  while ( !Renderer::WantsToQuit() && !appWantsToQuit )
  {
    Renderer::StartFrame();

    //////////////////////////////////////////////////////////////////////////
    // ImGui windows etc.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if ( ImGui::BeginMainMenuBar() )
    {
      if ( ImGui::BeginMenu( "File" ) )
      {
        if ( ImGui::MenuItem( "Open model..." ) )
        {
          // TODO: load model
        }
        ImGui::Separator();
        if ( ImGui::MenuItem( "Exit" ) )
        {
          appWantsToQuit = true;
        }
        ImGui::EndMenu();
      }
      if ( ImGui::BeginMenu( "Camera" ) )
      {
        ImGui::MenuItem( "Enable idle camera", NULL, &automaticCamera );
        ImGui::EndMenu();
      }
      if ( ImGui::BeginMenu( "Shaders" ) )
      {
        for ( int i = 0; shaders[ i ].name; i++ )
        {
          bool selected = &shaders[ i ] == current_shader;
          if ( ImGui::MenuItem( shaders[ i ].name, NULL, &selected ) )
          {
            load_shader( &shaders[ i ] );
          }
        }
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    if ( ImGui::Begin( "Camera" ) )
    {
      ImGui::DragFloat3( "Camera", (float *) &cameraPosition );
      ImGui::End();
    }

    ImGui::Render();

    //////////////////////////////////////////////////////////////////////////
    // Mesh render

    float verticalFovInRadian = 0.5f;
    projectionMatrix = glm::perspective( verticalFovInRadian, settings.nWidth / (float) settings.nHeight, 0.01f, glm::length( cameraPosition ) * 2.0f );
    Renderer::SetShaderConstant( "mat_projection", projectionMatrix );

    viewMatrix = glm::lookAtRH( cameraPosition, glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    Renderer::SetShaderConstant( "mat_view", viewMatrix );

    for ( std::map<int, Geometry::Mesh>::iterator it = Geometry::mMeshes.begin(); it != Geometry::mMeshes.end(); it++ )
    {
      glm::mat4x4 worldMatrix( 1.0f );
      Renderer::SetShaderConstant( "mat_world", worldMatrix );

      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, it->second.mIndexBufferObject );
      glBindBuffer( GL_ARRAY_BUFFER, it->second.mVertexBufferObject );
      glDrawElements( GL_TRIANGLES, it->second.mTriangleCount * 3, GL_UNSIGNED_INT, NULL );
    }

    //////////////////////////////////////////////////////////////////////////
    // End frame
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
    Renderer::EndFrame();
  }

  //////////////////////////////////////////////////////////////////////////
  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  Geometry::UnloadMesh();

  Renderer::Close();

  return 0;
}