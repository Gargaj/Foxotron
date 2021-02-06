
#include <cstdio>

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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef __APPLE__
#include "../TouchBar.h"
#endif

namespace Renderer
{

  GLFWwindow * mWindow = NULL;
  bool run = true;

  GLuint theShader = 0;
  GLuint glhVertexShader = 0;
  GLuint glhFullscreenQuadVB = 0;
  GLuint glhFullscreenQuadVA = 0;
  GLuint glhGUIVB = 0;
  GLuint glhGUIVA = 0;
  GLuint glhGUIProgram = 0;

  int nWidth = 0;
  int nHeight = 0;

  void MatrixOrthoOffCenterLH(float * pout, float l, float r, float b, float t, float zn, float zf)
  {
    memset( pout, 0, sizeof(float) * 4 * 4 );
    pout[0 + 0 * 4] = 2.0f / (r - l);
    pout[1 + 1 * 4] = 2.0f / (t - b);
    pout[2 + 2 * 4] = 1.0f / (zf -zn);
    pout[3 + 0 * 4] = -1.0f -2.0f *l / (r - l);
    pout[3 + 1 * 4] = 1.0f + 2.0f * t / (b - t);
    pout[3 + 2 * 4] = zn / (zn -zf);
    pout[3 + 3 * 4] = 1.0;
  }

  int readIndex = 0;
  int writeIndex = 1;
  GLuint pbo[2];

  static void error_callback(int error, const char *description) {
    switch (error) {
    case GLFW_API_UNAVAILABLE:
      printf("OpenGL is unavailable: ");
      break;
    case GLFW_VERSION_UNAVAILABLE:
      printf("OpenGL 4.1 (the minimum requirement) is not available: ");
      break;
    }
    printf("%s\n", description);
  }
  void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
  void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
  void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

  bool Open( RENDERER_SETTINGS * settings )
  {
    glfwSetErrorCallback(error_callback);
    theShader = 0;
    
#ifdef __APPLE__
    glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_FALSE);
#endif

    if(!glfwInit())
    {
      printf("[Renderer] GLFW init failed\n");
      return false;
    }
    printf("[GLFW] Version String: %s\n", glfwGetVersionString());

    nWidth = settings->nWidth;
    nHeight = settings->nHeight;

    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GLFW_FALSE);
#endif

    // TODO: change in case of resize support
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Prevent fullscreen window minimize on focus loss
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);

    GLFWmonitor *monitor = settings->windowMode == RENDERER_WINDOWMODE_FULLSCREEN ? glfwGetPrimaryMonitor() : NULL;

    mWindow = glfwCreateWindow(nWidth, nHeight, "BONZOMATIC - GLFW edition", monitor, NULL);
    if (!mWindow)
    {
      printf("[GLFW] Window creation failed\n");
      glfwTerminate();
      return false;
    }

#ifdef __APPLE__
#ifdef BONZOMATIC_ENABLE_TOUCHBAR
    ShowTouchBar(mWindow);
#endif
#endif
      
    glfwMakeContextCurrent(mWindow);

    // TODO: here add text callbacks
//     glfwSetKeyCallback(mWindow, key_callback);
//     glfwSetCharCallback(mWindow, character_callback);
    glfwSetCursorPosCallback(mWindow, cursor_position_callback);
    glfwSetMouseButtonCallback(mWindow, mouse_button_callback);
    glfwSetScrollCallback(mWindow, scroll_callback);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        printf("[GLFW] glewInit failed: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return false;
    }
    printf("[GLFW] Using GLEW %s\n", glewGetString(GLEW_VERSION));
    glGetError(); // reset glew error

    glfwSwapInterval(1);

#ifdef _WIN32
    if (settings->bVsync)
      wglSwapIntervalEXT(1);
#endif

    printf("[GLFW] OpenGL Version %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // Now, since OpenGL is behaving a lot in fullscreen modes, lets collect the real obtained size!
    printf("[GLFW] Requested framebuffer size: %d x %d\n", nWidth, nHeight);
    int fbWidth = 1;
    int fbHeight = 1;
    glfwGetFramebufferSize(mWindow, &fbWidth, &fbHeight);
    nWidth = settings->nWidth = fbWidth;
    nHeight = settings->nHeight = fbHeight;
    printf("[GLFW] Obtained framebuffer size: %d x %d\n", fbWidth, fbHeight);
    
    static float pFullscreenQuadVertices[] =
    {
      -1.0, -1.0,  0.5, 0.0, 0.0,
      -1.0,  1.0,  0.5, 0.0, 1.0,
       1.0, -1.0,  0.5, 1.0, 0.0,
       1.0,  1.0,  0.5, 1.0, 1.0,
    };

    glGenBuffers( 1, &glhFullscreenQuadVB );
    glBindBuffer( GL_ARRAY_BUFFER, glhFullscreenQuadVB );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 5 * 4, pFullscreenQuadVertices, GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glGenVertexArrays(1, &glhFullscreenQuadVA);

    glhVertexShader = glCreateShader( GL_VERTEX_SHADER );

    const char * szVertexShader =
      "#version 410 core\n"
      "in vec3 in_pos;\n"
      "in vec2 in_texcoord;\n"
      "out vec2 out_texcoord;\n"
      "void main()\n"
      "{\n"
      "  gl_Position = vec4( in_pos.x, in_pos.y, in_pos.z, 1.0 );\n"
      "  out_texcoord = in_texcoord;\n"
      "}";
    GLint nShaderSize = (GLint)strlen(szVertexShader);

    glShaderSource(glhVertexShader, 1, (const GLchar**)&szVertexShader, &nShaderSize);
    glCompileShader(glhVertexShader);

    GLint size = 0;
    GLint result = 0;
    char szErrorBuffer[5000];
    glGetShaderInfoLog(glhVertexShader, 4000, &size, szErrorBuffer);
    glGetShaderiv(glhVertexShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
      printf("[Renderer] Vertex shader compilation failed\n%s\n", szErrorBuffer);
      return false;
    }

#define GUIQUADVB_SIZE (1024 * 6)

    const char * defaultGUIVertexShader =
      "#version 410 core\n"
      "in vec3 in_pos;\n"
      "in vec4 in_color;\n"
      "in vec2 in_texcoord;\n"
      "in float in_factor;\n"
      "out vec4 out_color;\n"
      "out vec2 out_texcoord;\n"
      "out float out_factor;\n"
      "uniform vec2 v2Offset;\n"
      "uniform mat4 matProj;\n"
      "void main()\n"
      "{\n"
      "  vec4 pos = vec4( in_pos + vec3(v2Offset,0), 1.0 );\n"
      "  gl_Position = pos * matProj;\n"
      "  out_color = in_color;\n"
      "  out_texcoord = in_texcoord;\n"
      "  out_factor = in_factor;\n"
      "}\n";
    const char * defaultGUIPixelShader =
      "#version 410 core\n"
      "uniform sampler2D tex;\n"
      "in vec4 out_color;\n"
      "in vec2 out_texcoord;\n"
      "in float out_factor;\n"
      "out vec4 frag_color;\n"
      "void main()\n"
      "{\n"
      "  vec4 v4Texture = out_color * texture( tex, out_texcoord );\n"
      "  vec4 v4Color = out_color;\n"
      "  frag_color = mix( v4Texture, v4Color, out_factor );\n"
      "}\n";

    glhGUIProgram = glCreateProgram();

    GLuint vshd = glCreateShader(GL_VERTEX_SHADER);
    nShaderSize = (GLint)strlen(defaultGUIVertexShader);

    glShaderSource(vshd, 1, (const GLchar**)&defaultGUIVertexShader, &nShaderSize);
    glCompileShader(vshd);
    glGetShaderInfoLog(vshd, 4000, &size, szErrorBuffer);
    glGetShaderiv(vshd, GL_COMPILE_STATUS, &result);
    if (!result)
    {
      printf("[Renderer] Default GUI vertex shader compilation failed\n");
      return false;
    }

    GLuint fshd = glCreateShader(GL_FRAGMENT_SHADER);
    nShaderSize = (GLint)strlen(defaultGUIPixelShader);

    glShaderSource(fshd, 1, (const GLchar**)&defaultGUIPixelShader, &nShaderSize);
    glCompileShader(fshd);
    glGetShaderInfoLog(fshd, 4000, &size, szErrorBuffer);
    glGetShaderiv(fshd, GL_COMPILE_STATUS, &result);
    if (!result)
    {
      printf("[Renderer] Default GUI pixel shader compilation failed\n");
      return false;
    }

    glAttachShader(glhGUIProgram, vshd);
    glAttachShader(glhGUIProgram, fshd);
    glLinkProgram(glhGUIProgram);
    glGetProgramiv(glhGUIProgram, GL_LINK_STATUS, &result);
    if (!result)
    {
      return false;
    }

    glGenBuffers( 1, &glhGUIVB );
    glBindBuffer( GL_ARRAY_BUFFER, glhGUIVB );

    glGenVertexArrays(1, &glhGUIVA);

    //create PBOs to hold the data. this allocates memory for them too
    glGenBuffers(2, pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, nWidth * nHeight * sizeof(unsigned int), NULL, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, nWidth * nHeight * sizeof(unsigned int), NULL, GL_STREAM_READ);
    //unbind buffers for now
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    glViewport(0, 0, nWidth, nHeight);
    
    run = true;

    return true;
  }

  MouseEvent mouseEventBuffer[ 512 ];
  int mouseEventBufferCount = 0;
  void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
  {
    mouseEventBuffer[mouseEventBufferCount].eventType = MOUSEEVENTTYPE_MOVE;
    mouseEventBuffer[mouseEventBufferCount].x = xpos;
    mouseEventBuffer[mouseEventBufferCount].y = ypos;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) mouseEventBuffer[mouseEventBufferCount].button = MOUSEBUTTON_LEFT;
    mouseEventBufferCount++;
  }
  void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
  {
    if (action == GLFW_PRESS) {
      mouseEventBuffer[mouseEventBufferCount].eventType = MOUSEEVENTTYPE_DOWN;
    }
    else if (action == GLFW_RELEASE) {
      mouseEventBuffer[mouseEventBufferCount].eventType = MOUSEEVENTTYPE_UP;
    }
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouseEventBuffer[mouseEventBufferCount].x = xpos;
    mouseEventBuffer[mouseEventBufferCount].y = ypos;
    switch(button)
    {
      case GLFW_MOUSE_BUTTON_MIDDLE: mouseEventBuffer[mouseEventBufferCount].button = MOUSEBUTTON_MIDDLE; break;
      case GLFW_MOUSE_BUTTON_RIGHT:  mouseEventBuffer[mouseEventBufferCount].button = MOUSEBUTTON_RIGHT; break;
      case GLFW_MOUSE_BUTTON_LEFT:
      default:                mouseEventBuffer[mouseEventBufferCount].button = MOUSEBUTTON_LEFT; break;
    }
    mouseEventBufferCount++;
  }
  void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
  {
    mouseEventBuffer[mouseEventBufferCount].eventType = MOUSEEVENTTYPE_SCROLL;
    mouseEventBuffer[mouseEventBufferCount].x = xoffset;
    mouseEventBuffer[mouseEventBufferCount].y = yoffset;
    mouseEventBufferCount++;
  }

  void StartFrame()
  {
    glClearColor(0.08f, 0.18f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  }
  void EndFrame()
  {
    mouseEventBufferCount = 0;
    glfwSwapBuffers(mWindow);
    glfwPollEvents();
  }
  bool WantsToQuit()
  {
    return glfwWindowShouldClose(mWindow) || !run;
  }
  void Close()
  {
    glfwDestroyWindow(mWindow);
    glfwTerminate();
  }

  void RenderFullscreenQuad()
  {
    glBindVertexArray(glhFullscreenQuadVA);

    glUseProgram(theShader);

    glBindBuffer( GL_ARRAY_BUFFER, glhFullscreenQuadVB );

    const GLint position = glGetAttribLocation( theShader, "in_pos" );
    if (position >= 0)
    {
      glVertexAttribPointer( position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(0 * sizeof(GLfloat)) );
      glEnableVertexAttribArray( position );
    }

    const GLint texcoord = glGetAttribLocation( theShader, "in_texcoord" );
    if (texcoord >= 0)
    {
      glVertexAttribPointer( texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (GLvoid*)(3 * sizeof(GLfloat)) );
      glEnableVertexAttribArray( texcoord );
    }

    glBindBuffer( GL_ARRAY_BUFFER, glhFullscreenQuadVB );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

    if (texcoord >= 0)
      glDisableVertexAttribArray( texcoord );

    if (position >= 0)
      glDisableVertexAttribArray( position );

    glUseProgram(0);
  }

  bool ReloadShader( const char * szShaderCode, int nShaderCodeSize, char * szErrorBuffer, int nErrorBufferSize )
  {
    GLuint prg = glCreateProgram();
    GLuint shd = glCreateShader(GL_FRAGMENT_SHADER);
    GLint size = 0;
    GLint result = 0;

    glShaderSource(shd, 1, (const GLchar**)&szShaderCode, &nShaderCodeSize);
    glCompileShader(shd);
    glGetShaderInfoLog(shd, nErrorBufferSize, &size, szErrorBuffer);
    glGetShaderiv(shd, GL_COMPILE_STATUS, &result);
    if (!result)
    {
      glDeleteProgram(prg);
      glDeleteShader(shd);
      return false;
    }

    glAttachShader(prg, glhVertexShader);
    glAttachShader(prg, shd);
    glLinkProgram(prg);
    glGetProgramInfoLog(prg, nErrorBufferSize - size, &size, szErrorBuffer + size);
    glGetProgramiv(prg, GL_LINK_STATUS, &result);
    if (!result)
    {
      glDeleteProgram(prg);
      glDeleteShader(shd);
      return false;
    }

    if (theShader)
      glDeleteProgram(theShader);

    theShader = prg;

    return true;
  }

  void SetShaderConstant( const char * szConstName, float x )
  {
    GLint location = glGetUniformLocation( theShader, szConstName );
    if ( location != -1 )
    {
      glProgramUniform1f( theShader, location, x );
    }
  }

  void SetShaderConstant( const char * szConstName, float x, float y )
  {
    GLint location = glGetUniformLocation( theShader, szConstName );
    if ( location != -1 )
    {
      glProgramUniform2f( theShader, location, x, y );
    }
  }

  struct GLTexture : public Texture
  {
    GLuint ID;
    int unit;
  };

  int textureUnit = 0;

  Texture * CreateRGBA8Texture()
  {
    void * data = NULL;
    GLenum internalFormat = GL_SRGB8_ALPHA8;
    GLenum srcFormat = GL_FLOAT;
    GLenum format = GL_UNSIGNED_BYTE;

    GLuint glTexId = 0;
    glGenTextures( 1, &glTexId );
    glBindTexture( GL_TEXTURE_2D, glTexId );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    GLTexture * tex = new GLTexture();
    tex->width = nWidth;
    tex->height = nHeight;
    tex->ID = glTexId;
    tex->type = TEXTURETYPE_2D;
    tex->unit = textureUnit++;
    return tex;
  }

  Texture * CreateRGBA8TextureFromFile( const char * szFilename )
  {
    int comp = 0;
    int width = 0;
    int height = 0;
    void * data = NULL;
    GLenum internalFormat = GL_SRGB8_ALPHA8;
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
    if (!data) return NULL;

    GLuint glTexId = 0;
    glGenTextures( 1, &glTexId );
    glBindTexture( GL_TEXTURE_2D, glTexId );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, width, height, 0, srcFormat, format, data );

    stbi_image_free(data);

    GLTexture * tex = new GLTexture();
    tex->width = width;
    tex->height = height;
    tex->ID = glTexId;
    tex->type = TEXTURETYPE_2D;
    tex->unit = textureUnit++;
    return tex;
  }

  Texture * Create1DR32Texture( int w )
  {
    GLuint glTexId = 0;
    glGenTextures( 1, &glTexId );
    glBindTexture( GL_TEXTURE_1D, glTexId );

    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    float * data = new float[w];
    for ( int i = 0; i < w; ++i )
      data[i] = 0.0f;

    glTexImage1D( GL_TEXTURE_1D, 0, GL_R32F, w, 0, GL_RED, GL_FLOAT, data );

    delete[] data;

    glBindTexture( GL_TEXTURE_1D, 0 );

    GLTexture * tex = new GLTexture();
    tex->width = w;
    tex->height = 1;
    tex->ID = glTexId;
    tex->type = TEXTURETYPE_1D;
    tex->unit = textureUnit++;
    return tex;
  }

  void SetShaderTexture( const char * szTextureName, Texture * tex )
  {
    if (!tex)
      return;

    GLint location = glGetUniformLocation( theShader, szTextureName );
    if ( location != -1 )
    {
      glProgramUniform1i( theShader, location, ((GLTexture*)tex)->unit );
      glActiveTexture( GL_TEXTURE0 + ((GLTexture*)tex)->unit );
      switch( tex->type)
      {
        case TEXTURETYPE_1D: glBindTexture( GL_TEXTURE_1D, ((GLTexture*)tex)->ID ); break;
        case TEXTURETYPE_2D: glBindTexture( GL_TEXTURE_2D, ((GLTexture*)tex)->ID ); break;
      }
    }
  }

  bool UpdateR32Texture( Texture * tex, float * data )
  {
    glActiveTexture( GL_TEXTURE0 + ((GLTexture*)tex)->unit );
    glBindTexture( GL_TEXTURE_1D, ((GLTexture*)tex)->ID );
    glTexSubImage1D( GL_TEXTURE_1D, 0, 0, tex->width, GL_RED, GL_FLOAT, data );

    return true;
  }

  Texture * CreateA8TextureFromData( int w, int h, const unsigned char * data )
  {
    GLuint glTexId = 0;
    glGenTextures(1, &glTexId);
    glBindTexture(GL_TEXTURE_2D, glTexId);
    unsigned int * p32bitData = new unsigned int[ w * h ];
    for(int i=0; i<w*h; i++) p32bitData[i] = (data[i] << 24) | 0xFFFFFF;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, p32bitData);
    delete[] p32bitData;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLTexture * tex = new GLTexture();
    tex->width = w;
    tex->height = h;
    tex->ID = glTexId;
    tex->type = TEXTURETYPE_2D;
    tex->unit = 0; // this is always 0 cos we're not using shaders here
    return tex;
  }

  void ReleaseTexture( Texture * tex )
  {
    glDeleteTextures(1, &((GLTexture*)tex)->ID );
  }

  void CopyBackbufferToTexture( Texture * tex )
  {
    glActiveTexture( GL_TEXTURE0 + ( (GLTexture *) tex )->unit );
    glBindTexture( GL_TEXTURE_2D, ( (GLTexture *) tex )->ID );
    glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, nWidth, nHeight, 0 );
  }

  //////////////////////////////////////////////////////////////////////////
  // text rendering

  int nDrawCallCount = 0;
  Texture * lastTexture = NULL;
  void StartTextRendering()
  {
    glUseProgram(glhGUIProgram);
    glBindVertexArray(glhGUIVA);

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  }

  int bufferPointer = 0;
  unsigned char buffer[GUIQUADVB_SIZE * sizeof(float) * 7];
  bool lastModeIsQuad = true;
  void __FlushRenderCache()
  {
    if (!bufferPointer) return;

    glBindBuffer( GL_ARRAY_BUFFER, glhGUIVB );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float) * 7 * bufferPointer, buffer, GL_DYNAMIC_DRAW );

    GLuint position = glGetAttribLocation( glhGUIProgram, "in_pos" );
    glVertexAttribPointer( position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (GLvoid*)(0 * sizeof(GLfloat)) );
    glEnableVertexAttribArray( position );

    GLuint color = glGetAttribLocation( glhGUIProgram, "in_color" );
    glVertexAttribPointer( color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(float) * 7, (GLvoid*)(3 * sizeof(GLfloat)) );
    glEnableVertexAttribArray( color );

    GLuint texcoord = glGetAttribLocation( glhGUIProgram, "in_texcoord" );
    glVertexAttribPointer( texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (GLvoid*)(4 * sizeof(GLfloat)) );
    glEnableVertexAttribArray( texcoord );

    GLuint factor = glGetAttribLocation( glhGUIProgram, "in_factor" );
    glVertexAttribPointer( factor, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (GLvoid*)(6 * sizeof(GLfloat)) );
    glEnableVertexAttribArray( factor );

    if (lastModeIsQuad)
    {
      glDrawArrays( GL_TRIANGLES, 0, bufferPointer );
    }
    else
    {
      glDrawArrays( GL_LINES, 0, bufferPointer );
    }

    bufferPointer = 0;
  }
  void BindTexture( Texture * tex )
  {
    if (lastTexture != tex)
    {
      lastTexture = tex;
      if (tex)
      {
        __FlushRenderCache();

        GLint location = glGetUniformLocation( glhGUIProgram, "tex" );
        if ( location != -1 )
        {
          glProgramUniform1i( glhGUIProgram, location, ((GLTexture*)tex)->unit );
          glActiveTexture( GL_TEXTURE0 + ((GLTexture*)tex)->unit );
          switch( tex->type)
          {
            case TEXTURETYPE_1D: glBindTexture( GL_TEXTURE_1D, ((GLTexture*)tex)->ID ); break;
            case TEXTURETYPE_2D: glBindTexture( GL_TEXTURE_2D, ((GLTexture*)tex)->ID ); break;
          }
        }

      }
    }
  }
}
