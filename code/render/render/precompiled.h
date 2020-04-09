#pragma once
//------------------------------------------------------------------------------
/**
    @file render/precompiled.h
    
    Contains precompiled headers for render based on platform.
    
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/

// DirectX headers
#if _DEBUG
#define D3D_DEBUG_INFO (1)
#endif

#ifdef __VULKAN__
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#endif

#if __WIN32__
#include <xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif

#include <climits>
//------------------------------------------------------------------------------


