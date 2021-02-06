#include <stdio.h>

#include "Renderer.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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

bool load_mesh( const char * _path )
{
  Assimp::DefaultLogger::create( "", Assimp::Logger::VERBOSE );
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

  Assimp::DefaultLogger::kill();

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

  if ( !load_mesh( argv[ 1 ] ) )
  {
    return -3;
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

  Renderer::Close();

  return 0;
}