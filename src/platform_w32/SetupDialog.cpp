#include <windows.h>
#ifdef __MINGW32__
#include <stdio.h>
#endif
#include <tchar.h>
#include "../Renderer.h"
#include "../SetupDialog.h"
#include "resource.h"

#include <vector>
#include <algorithm>

namespace SetupDialog
{
class CSetupDialog;

CSetupDialog * pGlobal = NULL;

INT_PTR CALLBACK DlgFunc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

class CSetupDialog
{
public:
  typedef struct
  {
    int mWidth;
    int mHeight;
  } RESOLUTION;

  std::vector<RESOLUTION> mResolutions;
  HWND mHWndSetupDialog;

  CSetupDialog( void )
  {
    mHWndSetupDialog = NULL;
  }

  ~CSetupDialog( void )
  {
  }

  RENDERER_SETTINGS * mSetup;

  int __cdecl ResolutionSort( const void * a, const void * b )
  {
    RESOLUTION * aa = (RESOLUTION *) a;
    RESOLUTION * bb = (RESOLUTION *) b;
    if ( aa->mWidth < bb->mWidth ) return -1;
    if ( aa->mWidth > bb->mWidth ) return 1;
    if ( aa->mHeight < bb->mHeight ) return -1;
    if ( aa->mHeight > bb->mHeight ) return 1;
    return 0;
  }

  bool DialogProcedure( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
  {
    switch ( uMsg )
    {
      case WM_INITDIALOG:
        {
          mHWndSetupDialog = hWnd;

          int i = 0;
          while ( 1 )
          {
            DEVMODE d;
            BOOL h = EnumDisplaySettings( NULL, i++, &d );
            if ( !h ) break;

            //if ((d.dmPelsWidth * 9) / 16 != d.dmPelsHeight) continue;
            if ( d.dmBitsPerPel != 32 ) continue;

            if ( !mResolutions.size()
              || mResolutions[ mResolutions.size() - 1 ].mWidth != d.dmPelsWidth
              || mResolutions[ mResolutions.size() - 1 ].mHeight != d.dmPelsHeight )
            {
              RESOLUTION res;
              res.mWidth = d.dmPelsWidth;
              res.mHeight = d.dmPelsHeight;
              mResolutions.push_back( res );

            }
          }
          //std::sort(gaResolutions.begin(),gaResolutions.end(),&CSetupDialog::ResolutionSort);

          bool bFoundBest = false;
          for ( i = 0; i < mResolutions.size(); i++ )
          {
            TCHAR s[ 50 ];
            _sntprintf( s, 50, _T( "%d * %d" ), mResolutions[ i ].mWidth, mResolutions[ i ].mHeight );
            SendDlgItemMessage( hWnd, IDC_RESOLUTION, CB_ADDSTRING, 0, (LPARAM) s );

            if ( mResolutions[ i ].mWidth == mSetup->mWidth && mResolutions[ i ].mHeight == mSetup->mHeight )
            {
              SendDlgItemMessage( hWnd, IDC_RESOLUTION, CB_SETCURSEL, i, 0 );
              bFoundBest = true;
            }
            if ( !bFoundBest && mResolutions[ i ].mWidth == GetSystemMetrics( SM_CXSCREEN ) && mResolutions[ i ].mHeight == GetSystemMetrics( SM_CYSCREEN ) )
            {
              SendDlgItemMessage( hWnd, IDC_RESOLUTION, CB_SETCURSEL, i, 0 );
            }
          }

          if ( mSetup->mWindowMode == RENDERER_WINDOWMODE_FULLSCREEN )
          {
            SendDlgItemMessage( hWnd, IDC_FULLSCREEN, BM_SETCHECK, 1, 1 );
          }
          if ( mSetup->mVsync )
          {
            SendDlgItemMessage( hWnd, IDC_VSYNC, BM_SETCHECK, 1, 1 );
          }
          if ( mSetup->mMultisampling )
          {
            SendDlgItemMessage( hWnd, IDC_MULTISAMPLING, BM_SETCHECK, 1, 1 );
          }

          return true;
        } break;

      case WM_COMMAND:
        {
          switch ( LOWORD( wParam ) )
          {
            case IDOK:
              {
                mSetup->mWidth = mResolutions[ SendDlgItemMessage( hWnd, IDC_RESOLUTION, CB_GETCURSEL, 0, 0 ) ].mWidth;
                mSetup->mHeight = mResolutions[ SendDlgItemMessage( hWnd, IDC_RESOLUTION, CB_GETCURSEL, 0, 0 ) ].mHeight;
                mSetup->mWindowMode = SendDlgItemMessage( hWnd, IDC_FULLSCREEN, BM_GETCHECK, 0, 0 ) ? RENDERER_WINDOWMODE_FULLSCREEN : RENDERER_WINDOWMODE_WINDOWED;
                mSetup->mVsync = SendDlgItemMessage( hWnd, IDC_VSYNC, BM_GETCHECK, 0, 0 ) > 0;
                mSetup->mMultisampling = SendDlgItemMessage( hWnd, IDC_MULTISAMPLING, BM_GETCHECK, 0, 0 ) > 0;

                EndDialog( hWnd, TRUE );
              } break;
            case IDCANCEL:
              {
                EndDialog( hWnd, FALSE );
              } break;
          }
        } break;
    }
    return false;
  }

  bool Open( HINSTANCE hInstance, HWND hWnd )
  {
    return DialogBoxParam( hInstance, MAKEINTRESOURCE( IDD_SETUP ), hWnd, DlgFunc, (LPARAM) this ) != NULL;
  }
};

INT_PTR CALLBACK DlgFunc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  if ( uMsg == WM_INITDIALOG )
  {
    pGlobal = (CSetupDialog *) lParam; // todo: split to multiple hWnd-s! (if needed)
  }
  return pGlobal->DialogProcedure( hWnd, uMsg, wParam, lParam );
}

bool Open( RENDERER_SETTINGS * _settings )
{
  CSetupDialog dlg;
  dlg.mSetup = _settings;
  return dlg.Open( GetModuleHandle( NULL ), NULL );
}

}
