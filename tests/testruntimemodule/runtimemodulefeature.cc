//------------------------------------------------------------------------------
//  runtimemodulefeature.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/factory.h"
#include "game/featureunit.h"
#include "game/moduleinterface.h"
#include <cstdio>

namespace TestRuntimeModule
{

class RuntimeModuleFeature : public Game::FeatureUnit
{
    __DeclareClass(RuntimeModuleFeature)
public:
    virtual void OnAttach() override
    {
        Game::FeatureUnit::OnAttach();
        std::fprintf(stdout, "TestRuntimeModule: RuntimeModuleFeature attached\n");
    }

    virtual void OnActivate() override
    {
        Game::FeatureUnit::OnActivate();
        std::fprintf(stdout, "TestRuntimeModule: RuntimeModuleFeature activated\n");
    }

    virtual void OnDeactivate() override
    {
        std::fprintf(stdout, "TestRuntimeModule: RuntimeModuleFeature deactivated\n");
        Game::FeatureUnit::OnDeactivate();
    }

    virtual void OnRemove() override
    {
        std::fprintf(stdout, "TestRuntimeModule: RuntimeModuleFeature removed\n");
        Game::FeatureUnit::OnRemove();
    }
};

__ImplementClass(TestRuntimeModule::RuntimeModuleFeature, 'TRMF', Game::FeatureUnit);

} // namespace TestRuntimeModule

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
    outDescriptor->name = "testruntimemodule";
    outDescriptor->version = "0.1.0";
    outDescriptor->flags = 0;
    return 1;
}

NEBULA_MODULE_EXPORT void*
NebulaModuleCreateFeature()
{
    return Core::Factory::Instance()->Create(TestRuntimeModule::RuntimeModuleFeature::RTTI.GetName());
}

NEBULA_MODULE_EXPORT void
NebulaModuleDestroyFeature(void* feature)
{
    // Feature instances are currently managed by Nebula refcounting via Ptr.
    (void)feature;
}
