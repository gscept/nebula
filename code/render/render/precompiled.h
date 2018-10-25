#pragma once
//------------------------------------------------------------------------------
/**
    @file render/precompiled.h
    
    Contains precompiled headers for render based on platform.
    
    (C) 2017-2018 Individual contributors, see AUTHORS file
*/

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

#if __WIN32__
#include <xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif

#include <climits>
//------------------------------------------------------------------------------


