//------------------------------------------------------------------------------
//  navigationfeaturemodule.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "core/factory.h"
#include "game/moduleinterface.h"
#include "navigationfeature/navigationfeatureunit.h"

#if __WIN32__
#define NEBULA_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define NEBULA_MODULE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace
{
using NavigationFeatureRenderUiFn = void (*)(Graphics::GraphicsEntityId camera);
}

NEBULA_MODULE_EXPORT int
NebulaModuleGetDescriptor(NebulaModuleDescriptor* outDescriptor)
{
    if (outDescriptor == nullptr)
        return 0;

    outDescriptor->abiVersion = NEBULA_MODULE_ABI_VERSION;
    outDescriptor->name = "navigationfeaturemodule";
    outDescriptor->version = "0.1.0";
    outDescriptor->flags = 0;
    return 1;
}

NEBULA_MODULE_EXPORT void*
NebulaModuleCreateFeature()
{
    return Core::Factory::Instance()->Create(NavigationFeature::NavigationFeatureUnit::RTTI.GetName());
}

NEBULA_MODULE_EXPORT void
NebulaModuleDestroyFeature(void* feature)
{
    // Feature instances are managed by Nebula refcounting via Ptr.
    (void)feature;
}

NEBULA_MODULE_EXPORT void
NebulaNavigationFeatureRenderUI(Graphics::GraphicsEntityId camera)
{
    NavigationFeature::RenderUI(camera);
}
