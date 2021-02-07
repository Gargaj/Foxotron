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
extern const char * defaultShaderFilename;
extern const char defaultShader[ 65536 ];

extern int nWidth;
extern int nHeight;
extern GLFWwindow * mWindow;

bool Open( RENDERER_SETTINGS * settings );

void StartFrame();
void EndFrame();
bool WantsToQuit();

void RenderFullscreenQuad();

bool ReloadShaders( const char * szVertexShaderCode, int nVertexShaderCodeSize, const char * szFragmentShaderCode, int nFragmentShaderCodeSize, char * szErrorBuffer, int nErrorBufferSize );
void SetShaderConstant( const char * szConstName, float x );
void SetShaderConstant( const char * szConstName, float x, float y );
void SetShaderConstant( const char * szConstName, glm::vec3 & vector );
void SetShaderConstant( const char * szConstName, glm::mat4x4 & matrix );

void Close();

enum TEXTURETYPE
{
  TEXTURETYPE_1D = 1,
  TEXTURETYPE_2D = 2,
};

struct Texture
{
  int width;
  int height;
  TEXTURETYPE type;
};

Texture * CreateRGBA8Texture();
Texture * CreateRGBA8TextureFromFile( const char * szFilename );
Texture * CreateA8TextureFromData( int w, int h, const unsigned char * data );
Texture * Create1DR32Texture( int w );
bool UpdateR32Texture( Texture * tex, float * data );
void SetShaderTexture( const char * szTextureName, Texture * tex );
void BindTexture( Texture * tex ); // temporary function until all the quad rendering is moved to the renderer
void ReleaseTexture( Texture * tex );

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
} // namespace
