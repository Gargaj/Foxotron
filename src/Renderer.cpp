
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#define GLEW_NO_GLU
#include "GL/glew.h"
#ifdef _WIN32
#include <GL/wGLew.h>
#endif

#include "Renderer.h"
#include <string.h>

#include "stb_image.h"

namespace Renderer
{

GLFWwindow * mWindow = NULL;
bool run = true;

GLuint shaderProgram = 0;

int nWidth = 0;
int nHeight = 0;

static void error_callback( int error, const char * description )
{
  switch ( error )
  {
    case GLFW_API_UNAVAILABLE:
      printf( "OpenGL is unavailable: " );
      break;
    case GLFW_VERSION_UNAVAILABLE:
      printf( "OpenGL 4.1 (the minimum requirement) is not available: " );
      break;
  }
  printf( "%s\n", description );
}
void cursor_position_callback( GLFWwindow * window, double xpos, double ypos );
void mouse_button_callback( GLFWwindow * window, int button, int action, int mods );
void scroll_callback( GLFWwindow * window, double xoffset, double yoffset );
void drop_callback( GLFWwindow * window, int path_count, const char * paths[] );

bool Open( RENDERER_SETTINGS * settings )
{
  glfwSetErrorCallback( error_callback );
  shaderProgram = 0;

#ifdef __APPLE__
  glfwInitHint( GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE );
#endif

  if ( !glfwInit() )
  {
    printf( "[Renderer] GLFW init failed\n" );
    return false;
  }
  printf( "[GLFW] Version String: %s\n", glfwGetVersionString() );

  nWidth = settings->nWidth;
  nHeight = settings->nHeight;

  glfwWindowHint( GLFW_RED_BITS, 8 );
  glfwWindowHint( GLFW_GREEN_BITS, 8 );
  glfwWindowHint( GLFW_BLUE_BITS, 8 );
  glfwWindowHint( GLFW_ALPHA_BITS, 8 );
  glfwWindowHint( GLFW_DEPTH_BITS, 24 );
  glfwWindowHint( GLFW_STENCIL_BITS, 8 );

  glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );

  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
  glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

#ifdef __APPLE__
  glfwWindowHint( GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE );
  glfwWindowHint( GLFW_COCOA_GRAPHICS_SWITCHING, GLFW_FALSE );
#endif

  // TODO: change in case of resize support
  glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

  // Prevent fullscreen window minimize on focus loss
  glfwWindowHint( GLFW_AUTO_ICONIFY, GL_FALSE );

  GLFWmonitor * monitor = settings->windowMode == RENDERER_WINDOWMODE_FULLSCREEN ? glfwGetPrimaryMonitor() : NULL;

  mWindow = glfwCreateWindow( nWidth, nHeight, "FOXOTRON v0.0.1", monitor, NULL );
  if ( !mWindow )
  {
    printf( "[GLFW] Window creation failed\n" );
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent( mWindow );

  glfwSetDropCallback( mWindow, drop_callback );
  glfwSetCursorPosCallback( mWindow, cursor_position_callback );
  glfwSetMouseButtonCallback( mWindow, mouse_button_callback );
  glfwSetScrollCallback( mWindow, scroll_callback );

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if ( GLEW_OK != err )
  {
    printf( "[GLFW] glewInit failed: %s\n", glewGetErrorString( err ) );
    glfwTerminate();
    return false;
  }
  printf( "[GLFW] Using GLEW %s\n", glewGetString( GLEW_VERSION ) );
  glGetError(); // reset glew error

  glfwSwapInterval( 1 );

#ifdef _WIN32
  if ( settings->bVsync )
  {
    wglSwapIntervalEXT( 1 );
  }
#endif

  printf( "[GLFW] OpenGL Version %s, GLSL %s\n", glGetString( GL_VERSION ), glGetString( GL_SHADING_LANGUAGE_VERSION ) );

  // Now, since OpenGL is behaving a lot in fullscreen modes, lets collect the real obtained size!
  printf( "[GLFW] Requested framebuffer size: %d x %d\n", nWidth, nHeight );
  int fbWidth = 1;
  int fbHeight = 1;
  glfwGetFramebufferSize( mWindow, &fbWidth, &fbHeight );
  nWidth = settings->nWidth = fbWidth;
  nHeight = settings->nHeight = fbHeight;
  printf( "[GLFW] Obtained framebuffer size: %d x %d\n", fbWidth, fbHeight );

  glViewport( 0, 0, nWidth, nHeight );

  run = true;

  return true;
}

MouseEvent mouseEventBuffer[ 512 ];
int mouseEventBufferCount = 0;
std::string dropEventBuffer[ 512 ];
int dropEventBufferCount = 0;
void cursor_position_callback( GLFWwindow * window, double xpos, double ypos )
{
  mouseEventBuffer[ mouseEventBufferCount ].eventType = MOUSEEVENTTYPE_MOVE;
  mouseEventBuffer[ mouseEventBufferCount ].x = (float) xpos;
  mouseEventBuffer[ mouseEventBufferCount ].y = (float) ypos;
  if ( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS ) mouseEventBuffer[ mouseEventBufferCount ].button = MOUSEBUTTON_LEFT;
  mouseEventBufferCount++;
}
void mouse_button_callback( GLFWwindow * window, int button, int action, int mods )
{
  if ( action == GLFW_PRESS )
  {
    mouseEventBuffer[ mouseEventBufferCount ].eventType = MOUSEEVENTTYPE_DOWN;
  }
  else if ( action == GLFW_RELEASE )
  {
    mouseEventBuffer[ mouseEventBufferCount ].eventType = MOUSEEVENTTYPE_UP;
  }
  double xpos, ypos;
  glfwGetCursorPos( window, &xpos, &ypos );
  mouseEventBuffer[ mouseEventBufferCount ].x = (float) xpos;
  mouseEventBuffer[ mouseEventBufferCount ].y = (float) ypos;
  switch ( button )
  {
    case GLFW_MOUSE_BUTTON_MIDDLE: mouseEventBuffer[ mouseEventBufferCount ].button = MOUSEBUTTON_MIDDLE; break;
    case GLFW_MOUSE_BUTTON_RIGHT:  mouseEventBuffer[ mouseEventBufferCount ].button = MOUSEBUTTON_RIGHT; break;
    case GLFW_MOUSE_BUTTON_LEFT:
    default:                mouseEventBuffer[ mouseEventBufferCount ].button = MOUSEBUTTON_LEFT; break;
  }
  mouseEventBufferCount++;
}
void scroll_callback( GLFWwindow * window, double xoffset, double yoffset )
{
  mouseEventBuffer[ mouseEventBufferCount ].eventType = MOUSEEVENTTYPE_SCROLL;
  mouseEventBuffer[ mouseEventBufferCount ].x = (float) xoffset;
  mouseEventBuffer[ mouseEventBufferCount ].y = (float) yoffset;
  mouseEventBufferCount++;
}
void drop_callback( GLFWwindow * window, int path_count, const char * paths[] )
{
  for ( int i = 0; i < path_count; i++ )
  {
    dropEventBuffer[ dropEventBufferCount ] = paths[ i ];
    dropEventBufferCount++;
  }
}
void __SetupVertexArray( const char * name, int sizeInFloats, int & offsetInFloats )
{
  unsigned int stride = sizeof( float ) * 14;

  GLint location = glGetAttribLocation( shaderProgram, name );
  if ( location >= 0 )
  {
    glVertexAttribPointer( location, sizeInFloats, GL_FLOAT, GL_FALSE, stride, (GLvoid *) ( offsetInFloats * sizeof( GLfloat ) ) );
    glEnableVertexAttribArray( location );
  }

  offsetInFloats += sizeInFloats;
}

void StartFrame( glm::vec4 & clearColor )
{
  glClearColor( clearColor.r, clearColor.g, clearColor.b, clearColor.a );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

  glEnable( GL_DEPTH_TEST );
  glUseProgram( shaderProgram );
}

void RebindVertexArray()
{
  int offset = 0;
  __SetupVertexArray( "in_pos", 3, offset );
  __SetupVertexArray( "in_normal", 3, offset );
  __SetupVertexArray( "in_tangent", 3, offset );
  __SetupVertexArray( "in_binormal", 3, offset );
  __SetupVertexArray( "in_texcoord", 2, offset );
}

void EndFrame()
{
  mouseEventBufferCount = 0;
  dropEventBufferCount = 0;
  glfwSwapBuffers( mWindow );
  glfwPollEvents();
}

bool WantsToQuit()
{
  return glfwWindowShouldClose( mWindow ) || !run;
}
void Close()
{
  glfwDestroyWindow( mWindow );
  glfwTerminate();
}

bool ReloadShaders( const char * szVertexShaderCode, int nVertexShaderCodeSize, const char * szFragmentShaderCode, int nFragmentShaderCodeSize, char * szErrorBuffer, int nErrorBufferSize )
{
  GLuint program = glCreateProgram();

  GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
  GLint size = 0;
  GLint result = 0;

  //////////////////////////////////////////////////////////////////////////
  // Vertex shader
  glShaderSource( vertexShader, 1, (const GLchar **) &szVertexShaderCode, &nVertexShaderCodeSize );
  glCompileShader( vertexShader );
  glGetShaderInfoLog( vertexShader, nErrorBufferSize, &size, szErrorBuffer );
  glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &result );
  if ( !result )
  {
    glDeleteProgram( program );
    glDeleteShader( vertexShader );
    return false;
  }

  GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

  //////////////////////////////////////////////////////////////////////////
  // Fragment shader
  glShaderSource( fragmentShader, 1, (const GLchar **) &szFragmentShaderCode, &nFragmentShaderCodeSize );
  glCompileShader( fragmentShader );
  glGetShaderInfoLog( fragmentShader, nErrorBufferSize, &size, szErrorBuffer );
  glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &result );
  if ( !result )
  {
    glDeleteProgram( program );
    glDeleteShader( fragmentShader );
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  // Link shaders to program
  glAttachShader( program, vertexShader );
  glAttachShader( program, fragmentShader );
  glLinkProgram( program );
  glGetProgramInfoLog( program, nErrorBufferSize - size, &size, szErrorBuffer + size );
  glGetProgramiv( program, GL_LINK_STATUS, &result );
  if ( !result )
  {
    glDeleteProgram( program );
    glDeleteShader( fragmentShader );
    return false;
  }

  if ( shaderProgram )
  {
    glDeleteProgram( shaderProgram );
  }

  shaderProgram = program;

  return true;
}

void SetShaderConstant( const char * szConstName, bool x )
{
  GLint location = glGetUniformLocation( shaderProgram, szConstName );
  if ( location != -1 )
  {
    glProgramUniform1i( shaderProgram, location, x ? 1 : 0 );
  }
}

void SetShaderConstant( const char * szConstName, float x )
{
  GLint location = glGetUniformLocation( shaderProgram, szConstName );
  if ( location != -1 )
  {
    glProgramUniform1f( shaderProgram, location, x );
  }
}

void SetShaderConstant( const char * szConstName, float x, float y )
{
  GLint location = glGetUniformLocation( shaderProgram, szConstName );
  if ( location != -1 )
  {
    glProgramUniform2f( shaderProgram, location, x, y );
  }
}

void SetShaderConstant( const char * szConstName, const glm::vec3 & vector )
{
  GLint location = glGetUniformLocation( shaderProgram, szConstName );
  if ( location != -1 )
  {
    glProgramUniform3f( shaderProgram, location, vector.x, vector.y, vector.z );
  }
}

void SetShaderConstant( const char * szConstName, const glm::vec4 & vector )
{
  GLint location = glGetUniformLocation( shaderProgram, szConstName );
  if ( location != -1 )
  {
    glProgramUniform4f( shaderProgram, location, vector.x, vector.y, vector.z, vector.w );
  }
}

void SetShaderConstant( const char * szConstName, const glm::mat4x4 & matrix )
{
  GLint location = glGetUniformLocation( shaderProgram, szConstName );
  if ( location != -1 )
  {
    glProgramUniformMatrix4fv( shaderProgram, location, 1, 0, (float*)&matrix );
  }
}

int textureUnit = 0;

Texture * CreateRGBA8TextureFromFile( const char * szFilename, const bool _loadAsSRGB /*= false*/ )
{
  int comp = 0;
  int width = 0;
  int height = 0;
  void * data = NULL;
  GLenum internalFormat = _loadAsSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
  GLenum srcFormat = GL_RGBA;
  GLenum format = GL_UNSIGNED_BYTE;
  if ( stbi_is_hdr( szFilename ) )
  {
    internalFormat = GL_RGBA32F;
    format = GL_FLOAT;
    data = stbi_loadf( szFilename, &width, &height, &comp, STBI_rgb_alpha );
  }
  else
  {
    data = stbi_load( szFilename, &width, &height, &comp, STBI_rgb_alpha );
  }
  if ( !data ) return NULL;

  GLuint glTexId = 0;
  glGenTextures( 1, &glTexId );
  glBindTexture( GL_TEXTURE_2D, glTexId );

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, width, height, 0, srcFormat, format, data );

  stbi_image_free( data );

  Texture * tex = new Texture();
  tex->mWidth = width;
  tex->mHeight = height;
  tex->mType = TEXTURETYPE_2D;
  tex->mFilename = szFilename;
  tex->mGLTextureID = glTexId;
  tex->mGLTextureUnit = textureUnit++;
  return tex;
}

void SetShaderTexture( const char * szTextureName, Texture * tex )
{
  if ( !tex )
    return;

  GLint location = glGetUniformLocation( shaderProgram, szTextureName );
  if ( location != -1 )
  {
    glProgramUniform1i( shaderProgram, location, ( (Texture *) tex )->mGLTextureUnit );
    glActiveTexture( GL_TEXTURE0 + ( (Texture *) tex )->mGLTextureUnit );
    switch ( tex->mType )
    {
      case TEXTURETYPE_1D: glBindTexture( GL_TEXTURE_1D, ( (Texture *) tex )->mGLTextureID ); break;
      case TEXTURETYPE_2D: glBindTexture( GL_TEXTURE_2D, ( (Texture *) tex )->mGLTextureID ); break;
    }
  }
}


void ReleaseTexture( Texture * tex )
{
  glDeleteTextures( 1, &( (Texture *) tex )->mGLTextureID );
}

void CopyBackbufferToTexture( Texture * tex )
{
  glActiveTexture( GL_TEXTURE0 + ( (Texture *) tex )->mGLTextureUnit );
  glBindTexture( GL_TEXTURE_2D, ( (Texture *) tex )->mGLTextureID );
  glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, nWidth, nHeight, 0 );
}

}// namespace Renderer
