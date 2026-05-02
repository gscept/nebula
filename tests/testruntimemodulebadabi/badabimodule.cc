//------------------------------------------------------------------------------
//  badabimodule.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "game/moduleinterface.h"

NEBULA_MODULE_EXPORT int
NebulaModuleGetDescriptor(NebulaModuleDescriptor* outDescriptor)
{
    if (outDescriptor == nullptr)
        return 0;

    outDescriptor->abiVersion = NEBULA_MODULE_ABI_VERSION + 42;
    outDescriptor->name = "testruntimemodulebadabi";
    outDescriptor->version = "0.1.0";
    outDescriptor->flags = 0;
    return 1;
}

NEBULA_MODULE_EXPORT void*
NebulaModuleCreateFeature()
{
    return nullptr;
}

NEBULA_MODULE_EXPORT void
NebulaModuleDestroyFeature(void* feature)
{
    (void)feature;
}
