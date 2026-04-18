//------------------------------------------------------------------------------
//  editorfeaturemodule.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "core/factory.h"
#include "game/moduleinterface.h"
#include "editorfeature/editorfeatureunit.h"
#include "cr/cr.h"

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
    outDescriptor->name = "editorfeaturemodule";
    outDescriptor->version = "0.1.0";
    outDescriptor->flags = 0;
    return 1;
}

NEBULA_MODULE_EXPORT void*
NebulaModuleCreateFeature()
{
    return Core::Factory::Instance()->Create(EditorFeature::EditorFeatureUnit::RTTI.GetName());
}

NEBULA_MODULE_EXPORT void
NebulaModuleDestroyFeature(void* feature)
{
    (void)feature;
}

//------------------------------------------------------------------------------
// cr plugin entry point — handles plugin lifecycle events from a cr host.
//------------------------------------------------------------------------------
CR_EXPORT int
cr_main(struct cr_plugin* ctx, enum cr_op operation)
{
    (void)ctx;
    switch (operation)
    {
        case CR_LOAD:
            // Module just loaded or reloaded; nothing to restore for now.
            break;
        case CR_UNLOAD:
            // About to be unloaded for a reload; flush any pending work here.
            break;
        case CR_CLOSE:
            // Final shutdown; nothing extra needed — Nebula module teardown
            // is handled via NebulaModuleDestroyFeature / OnDeactivate.
            break;
        default:
            break;
    }
    return 0;
}
