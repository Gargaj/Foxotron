#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <glm.hpp>

typedef enum
{
  RENDERER_WINDOWMODE_WINDOWED = 0,
  RENDERER_WINDOWMODE_FULLSCREEN,
  RENDERER_WINDOWMODE_BORDERLESS
} RENDERER_WINDOWMODE;

typedef struct
{
  int nWidth;
  int nHeight;
  RENDERER_WINDOWMODE windowMode;
  bool bVsync;
} RENDERER_SETTINGS;

namespace Renderer
{
enum TEXTURETYPE
{
  TEXTURETYPE_1D = 1,
  TEXTURETYPE_2D = 2,
};

struct Texture
{
  int mWidth;
  int mHeight;
  TEXTURETYPE mType;
  std::string mFilename;
  unsigned int mGLTextureID;
  int mGLTextureUnit;
};

struct Shader
{
  unsigned int mProgram;
  unsigned int mVertexShader;
  unsigned int mFragmentShader;
  void SetConstant( const char * szConstName, bool x );
  void SetConstant( const char * szConstName, float x );
  void SetConstant( const char * szConstName, float x, float y );
  void SetConstant( const char * szConstName, const glm::vec3 & vector );
  void SetConstant( const char * szConstName, const glm::vec4 & vector );
  void SetConstant( const char * szConstName, const glm::mat4x4 & matrix );
  void SetTexture( const char * szTextureName, Texture * tex );
};

extern const char * defaultShaderFilename;
extern const char defaultShader[ 65536 ];

extern int nWidth;
extern int nHeight;
extern GLFWwindow * mWindow;

bool Open( RENDERER_SETTINGS * settings );

void StartFrame( glm::vec4 & clearColor );
void RebindVertexArray();
void EndFrame();
bool WantsToQuit();

void RenderFullscreenQuad();

Shader * CreateShader( const char * szVertexShaderCode, int nVertexShaderCodeSize, const char * szFragmentShaderCode, int nFragmentShaderCodeSize, char * szErrorBuffer, int nErrorBufferSize );
void ReleaseShader( Shader * _shader );

void Close();

Texture * CreateRGBA8TextureFromFile( const char * szFilename, const bool _loadAsSRGB = false );
Texture * CreateRG32FTextureFromRawFile( const char* szFilename, int width, int height );
void ReleaseTexture( Texture * tex );

void SetShader( Shader * _shader );

enum MOUSEEVENTTYPE
{
  MOUSEEVENTTYPE_DOWN = 0,
  MOUSEEVENTTYPE_MOVE,
  MOUSEEVENTTYPE_UP,
  MOUSEEVENTTYPE_SCROLL
};
enum MOUSEBUTTON
{
  MOUSEBUTTON_LEFT = 0,
  MOUSEBUTTON_RIGHT,
  MOUSEBUTTON_MIDDLE,
};
struct MouseEvent
{
  MOUSEEVENTTYPE eventType;
  float x;
  float y;
  MOUSEBUTTON button;
};
extern MouseEvent mouseEventBuffer[ 512 ];
extern int mouseEventBufferCount;
extern std::string dropEventBuffer[ 512 ];
extern int dropEventBufferCount;
} // namespace
