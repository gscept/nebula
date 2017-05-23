#pragma once
//------------------------------------------------------------------------------
/**
    @file core/win32/precompiled.h
    
    Contains precompiled headers on the Win32 platform.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WIN32_WINNT 0x0503

#define NOGDICAPMASKS
#define OEMRESOURCE
#define NOATOM
// clashes with shlobj.h
//#define NOCTLMGR
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSERVICE
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

// Windows headers
#include <windows.h>
#include <winbase.h>
#include <process.h>
#include <shlobj.h>
#include <tchar.h>
#include <strsafe.h>
#include <wininet.h>
#include <winsock2.h>
#include <rpc.h>
#if _MSC_VER == 1900
#pragma warning ( push )
#pragma warning ( disable : 4091)
#endif
#include <dbghelp.h>
#if _MSC_VER == 1900
#pragma warning (pop)
#endif
#include <intrin.h>

#if __USE_XNA
// compile xna-math with sse/sse2 support for win32, to disable
// it and run completely in floating point unit, uncomment the following line 
// #define _XM_NO_INTRINSICS_ 
#include <xnamath.h>
#endif

// crt headers
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>

#include <stdint.h>

// DirectX headers
#if _DEBUG
#define D3D_DEBUG_INFO (1)
#endif

#ifdef __VULKAN__
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#endif

#ifdef __OGL4__
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/GLU.h>
#include "GLFW/glfw3.h"
#if _DEBUG
	//#define GLSUCCESS glGetError() == GL_NO_ERROR
    #define GLSUCCESS true
	#define ASSERTGLSUCCESS \
		{\
			GLenum err = glGetError(); \
			if (err != GL_NO_ERROR) \
		{ \
			n_error((const char*)gluErrorString(err)); \
		}\
		}
#else
	#define GLSUCCESS GL_TRUE
	#define ASSERTGLSUCCESS
#endif
#define ILSUCCESS ilGetError()  == IL_NO_ERROR
#endif

#if _MSC_VER == 1700
	#pragma warning ( disable : 4005 )
#endif

#ifdef __DX9__
#include <d3d9.h>
#include <d3dx9.h>
#endif

#ifdef __DX11__
#include <d3dx11.h>
#include <d3d11.h>
#include <d3dx11effect.h>
#endif

#if defined(__DX9__) || defined(__DX11__)
#include <dxgi.h>
#include <dxerr.h>
#include <d3dx9math.h>
#include <D2D1.h>
#include <DWrite.h>
#endif


#include <xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
//------------------------------------------------------------------------------


