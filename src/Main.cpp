#include <stdio.h>

#include "Renderer.h"
#include "Geometry.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>


struct Shader
{
  const char * name;
  const char * vertexShaderPath;
  const char * fragmentShaderPath;
} shaders[] = {
  { "Basic SpecGloss", "Shaders/basic_specgloss.fs", "Shaders/basic_specgloss.fs" },
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

    ImGui::Render();

    //////////////////////////////////////////////////////////////////////////
    // Mesh render

    // doo doo dooooooooo

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