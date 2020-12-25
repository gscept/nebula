#pragma once
//------------------------------------------------------------------------------
/**
    @file core/config.h

    Nebula compiler specific defines and configuration.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/

// setup platform identification macros
#ifdef __WIN32__
#undef __WIN32__
#endif
#if (WIN32 || _WIN32)
#define __WIN32__ (1)
#endif

#ifdef __GNUC__
#ifndef __LINUX__
#define __LINUX__ (1)
#endif
#endif

//------------------------------------------------------------------------------
/**
    Nebula configuration.
*/

#ifdef _DEBUG
#define NEBULA_DEBUG (1)
#endif

/// max size of a data slice is 16 kByte - 1 byte
/// this needs to be in a header, which is accessable from SPU too,
/// thats why its here
static const int JobMaxSliceSize = 0xFFFF;

#if PUBLIC_BUILD
#define __NEBULA_NO_ASSERT__ (0)    // DON'T SET THIS TO (1) - PUBLIC_BUILD SHOULD STILL DISPLAY ASSERTS!
#else
#define __NEBULA_NO_ASSERT__ (0)
#endif

// define whether a platform comes with archive support built into the OS
#define NEBULA_NATIVE_ARCHIVE_SUPPORT (0)

// enable/disable Nebula memory stats
#if NEBULA_DEBUG
#if __WIN32__
#define NEBULA_MEMORY_STATS (1)
#else
#define NEBULA_MEMORY_STATS (0)
#endif
#define NEBULA_MEMORY_ADVANCED_DEBUGGING (0)
#else
#define NEBULA_MEMORY_STATS (0) 
#define NEBULA_MEMORY_ADVANCED_DEBUGGING (0)
#endif

// enable/disable memory pool allocation for refcounted object
// FIXME -> memory pool is disabled for all platforms, cause it causes crashes
#if (__MAYA__ || __WIN32__ )
#define NEBULA_OBJECTS_USE_MEMORYPOOL (0)
#else
#define NEBULA_OBJECTS_USE_MEMORYPOOL (0)
#endif

// enable/disable thread-local StringAtom tables
#if (__LINUX__)
#define NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES (0)
#else
#define NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES (1)
#endif

// enable/disable growth of StringAtom buffer
#define NEBULA_ENABLE_GLOBAL_STRINGBUFFER_GROWTH (1)

// size of of a chunk of the global string buffer for StringAtoms
#define NEBULA_GLOBAL_STRINGBUFFER_CHUNKSIZE (32 * 1024)

// enable/disable Nebula animation system log messages
#define NEBULA_ANIMATIONSYSTEM_VERBOSELOG (0)
#define NEBULA_ANIMATIONSYSTEM_FRAMEDUMP (0)

// enable/disable bounds checking in the container util classes
#if NEBULA_DEBUG
#define NEBULA_BOUNDSCHECKS (1)
#else
#define NEBULA_BOUNDSCHECKS (0)
#endif

// enable/disable the builtin HTTP server
#if PUBLIC_BUILD
#define __NEBULA_HTTP__ (0)
#else
#define __NEBULA_HTTP__ (1)
#endif

// enable/disable profiling (see Debug::DebugTimer, Debug::DebugCounter)
#if PUBLIC_BUILD
    #define NEBULA_ENABLE_PROFILING (0)
#elif __NEBULA_HTTP__
// profiling needs http
    #define NEBULA_ENABLE_PROFILING (1)
#else
    #define NEBULA_ENABLE_PROFILING (0)
#endif

// max length of a path name
#define NEBULA_MAXPATH (512)

// enable/disable support for Nebula2 file formats and concepts
#define NEBULA_LEGACY_SUPPORT (1)

// enable/disable mini dumps
#define NEBULA_ENABLE_MINIDUMPS (0)

// Nebula's main window class
#define NEBULA_WINDOW_CLASS "Nebula::MainWindow"

// number of lines in the IO::HistoryConsoleHandler ring buffer
#define NEBULA_CONSOLE_HISTORY_SIZE (256)

// enable legacy support for 3-component vectors in XML files
#define NEBULA_XMLREADER_LEGACY_VECTORS (1)

// define the standard IO scheme for the platform
#define DEFAULT_IO_SCHEME "file"

#if 0
#FIXME add defines for these in cmake instead
#if __VULKAN__
#define NEBULA_DEFAULT_FRAMESHADER_NAME "vkdefault"
#else
#error "No default frameshader defined for platform"
#endif
#endif

// default resource names
#if __WIN32__ || __LINUX__
#define NEBULA_TEXTURE_EXTENSION ".dds"
#define NEBULA_SURFACE_EXTENSION ".sur"
#define NEBULA_MESH_EXTENSION ".nvx2"
#endif

// VisualStudio settings
#ifdef _MSC_VER
#define __VC__ (1)
#endif

// GCC settings
#if defined __GNUC__
#define __cdecl
#define __forceinline inline __attribute__((always_inline))
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

#if !defined(__GNUC__)
#define  __attribute__(x)  /**/
#endif

// define max texture space for resource streaming
#if __WIN32__
// 512 MB
#define __maxTextureBytes__ (524288000)
#else
// 256 MB
#define __maxTextureBytes__ (268435456)
#endif

#define NEBULA_THREAD_DEFAULTSTACKSIZE 65536

//------------------------------------------------------------------------------
