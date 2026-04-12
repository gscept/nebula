//------------------------------------------------------------------------------
//  badexportsmodule.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/moduleinterface.h"

#if __WIN32__
#define NEBULA_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define NEBULA_MODULE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

NEBULA_MODULE_EXPORT int
NebulaModuleGetDescriptor(NebulaModuleDescriptor* outDescriptor)
{
    if (outDescriptor == nullptr)
        return 0;

    outDescriptor->abiVersion = NEBULA_MODULE_ABI_VERSION;
    outDescriptor->name = "testruntimemodulebadexports";
    outDescriptor->version = "0.1.0";
    outDescriptor->flags = 0;
    return 1;
}
