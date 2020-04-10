#pragma once
//------------------------------------------------------------------------------
/**
    @file core/win32/precompiled.h
    
    Contains precompiled headers on the Win platform.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WIN32_WINNT 0x0601

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
#define NOMINMAX

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
#if _MSC_VER >= 1910
#include <new>
#endif
#include <intrin.h>

#if __USE_MATH_DIRECTX
#include <DirectXMath.h>
#endif

// crt headers
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>

#include <stdint.h>