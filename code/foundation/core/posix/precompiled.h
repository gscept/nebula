#pragma once
#ifndef CORE_POSIX_PRECOMPILED_H
#define CORE_POSIX_PRECOMPILED_H
//------------------------------------------------------------------------------
/**
    @file core/win32/precompiled.h
    
    Contains precompiled headers on the Posix platform.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
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
#include <cfloat>
#include <limits.h>

// sse intrinsics
#if __SSE2__ || __SSE4_1__ || __AVX2__
#include <emmintrin.h>
#include <immintrin.h>
#elif __aarc64__
#include <arm_neon.h>
#endif

//------------------------------------------------------------------------------
#endif
