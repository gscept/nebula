//------------------------------------------------------------------------------
/**
    @file render/stdneb.h

    Precompiled header. Put platform-specific headers which rarely change
    in here (e.g. windows.h).

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __WIN32__
#include "core/win32/precompiled.h"
#elif __linux__
#include "core/posix/precompiled.h"
#else
#error "precompiled.h not implemented on this platform"
#endif

#include "render/precompiled.h"
//------------------------------------------------------------------------------
