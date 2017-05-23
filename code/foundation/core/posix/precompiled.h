#pragma once
#ifndef CORE_POSIX_PRECOMPILED_H
#define CORE_POSIX_PRECOMPILED_H
//------------------------------------------------------------------------------
/**
    @file core/win32/precompiled.h
    
    Contains precompiled headers on the Posix platform.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file    
*/

// crt headers
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <cstdint>
#include <pthread.h>
#include <errno.h>

// sse intrinsics
#include <emmintrin.h>
#include <immintrin.h>

#ifdef __OGL4__
#include <GL/glew.h>
#include <GL/glu.h>
#include "GLFW/glfw3.h"
#define GLSUCCESS glGetError() == GL_NO_ERROR 
#endif

#ifdef __VULKAN__
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#endif

//------------------------------------------------------------------------------
#endif
