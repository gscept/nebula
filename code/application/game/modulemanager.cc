//------------------------------------------------------------------------------
//  modulemanager.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "game/modulemanager.h"

#include "game/featureunit.h"
#include "game/gameserver.h"
#include "io/fswrapper.h"
#include "io/ioserver.h"
#include "system/library.h"

namespace Game
{
__ImplementClass(Game::ModuleManager, 'GMDM', Core::RefCounted);

struct ModuleManager::LoadedModule
{
    RuntimeModuleConfig config;
    Base::Library* library;
    Ptr<FeatureUnit> feature;
};

//------------------------------------------------------------------------------
/**
*/
ModuleManager::ModuleManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModuleManager::~ModuleManager()
{
    n_assert(this->loadedModules.IsEmpty());
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::LoadModules(const Util::Array<RuntimeModuleConfig>& moduleConfigs, GameServer* gameServer, bool strictMode)
{
    n_assert(gameServer != nullptr);
    bool success = true;
    for (IndexT i = 0; i < moduleConfigs.Size(); i++)
    {
        const RuntimeModuleConfig& moduleConfig = moduleConfigs[i];
        if (!moduleConfig.enabled)
            continue;

        if (!this->LoadModule(moduleConfig, gameServer, strictMode))
        {
            success = false;
            if (strictMode || moduleConfig.required)
                break;
        }
    }
    return success;
}

//------------------------------------------------------------------------------
/**
*/
void
ModuleManager::UnloadModules(GameServer* gameServer)
{
    if (gameServer == nullptr)
        return;

    for (IndexT i = this->loadedModules.Size() - 1; i >= 0; i--)
    {
        this->UnloadModule(this->loadedModules[i], gameServer);
    }
    this->loadedModules.Clear();
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::IsModuleLoaded(const Util::String& moduleName) const
{
    return this->FindLoadedModuleIndex(moduleName) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ModuleManager::GetNumLoadedModules() const
{
    return this->loadedModules.Size();
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::LoadModule(const RuntimeModuleConfig& moduleConfig, GameServer* gameServer, bool strictMode)
{
    if (moduleConfig.name.IsValid() && this->IsModuleLoaded(moduleConfig.name))
    {
        n_printf("ModuleManager: module '%s' is already loaded, skipping duplicate request\n", moduleConfig.name.AsCharPtr());
        return true;
    }

    Util::String libraryPath = this->ResolveLibraryPath(moduleConfig);
    if (!libraryPath.IsValid())
    {
        n_warning("ModuleManager: module '%s' has no valid path\n", moduleConfig.name.AsCharPtr());
        return !(strictMode || moduleConfig.required);
    }

    Base::Library* library = new System::Library();
    library->SetPath(IO::URI(libraryPath));
    if (!library->Load())
    {
        delete library;
        return !(strictMode || moduleConfig.required);
    }

    NebulaModuleGetDescriptorFn getDescriptor = reinterpret_cast<NebulaModuleGetDescriptorFn>(library->GetExport(NEBULA_MODULE_GET_DESCRIPTOR_EXPORT));
    NebulaModuleCreateFeatureFn createFeature = reinterpret_cast<NebulaModuleCreateFeatureFn>(library->GetExport(NEBULA_MODULE_CREATE_FEATURE_EXPORT));
    NebulaModuleDestroyFeatureFn destroyFeature = reinterpret_cast<NebulaModuleDestroyFeatureFn>(library->GetExport(NEBULA_MODULE_DESTROY_FEATURE_EXPORT));

    if (getDescriptor == nullptr || createFeature == nullptr)
    {
        n_warning("ModuleManager: module '%s' is missing required exports\n", moduleConfig.name.AsCharPtr());
        library->Close();
        delete library;
        return !(strictMode || moduleConfig.required);
    }

    NebulaModuleDescriptor desc = {};
    if (getDescriptor(&desc) == 0)
    {
        n_warning("ModuleManager: module '%s' descriptor callback failed\n", moduleConfig.name.AsCharPtr());
        library->Close();
        delete library;
        return !(strictMode || moduleConfig.required);
    }

    if (desc.abiVersion != NEBULA_MODULE_ABI_VERSION)
    {
        n_warning("ModuleManager: module '%s' has ABI %u, expected %u\n", moduleConfig.name.AsCharPtr(), desc.abiVersion, NEBULA_MODULE_ABI_VERSION);
        library->Close();
        delete library;
        return !(strictMode || moduleConfig.required);
    }

    FeatureUnit* featureRaw = reinterpret_cast<FeatureUnit*>(createFeature());
    if (featureRaw == nullptr)
    {
        n_warning("ModuleManager: module '%s' did not return a feature instance\n", moduleConfig.name.AsCharPtr());
        library->Close();
        delete library;
        return !(strictMode || moduleConfig.required);
    }

    Ptr<FeatureUnit> feature = featureRaw;
    gameServer->AttachGameFeature(feature);

    LoadedModule loaded;
    loaded.config = moduleConfig;
    loaded.library = library;
    loaded.feature = feature;
    this->loadedModules.Append(loaded);

    const char* descName = desc.name != nullptr ? desc.name : "<unnamed>";
    const char* descVersion = desc.version != nullptr ? desc.version : "<unknown>";
    if (destroyFeature != nullptr)
    {
        n_warning("ModuleManager: module '%s' exports '%s', but runtime expects FeatureUnit instances to be managed via Nebula refcounting\n", descName, NEBULA_MODULE_DESTROY_FEATURE_EXPORT);
    }
    n_printf("ModuleManager: loaded module '%s' v%s from '%s'\n", descName, descVersion, libraryPath.AsCharPtr());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::UnloadModule(LoadedModule& loaded, GameServer* gameServer)
{
    if (loaded.feature.isvalid())
    {
        gameServer->RemoveGameFeature(loaded.feature);

        // Keep the module resident if external references still hold the feature.
        // Unloading the library in this state could leave dangling vtables.
        if (loaded.feature->GetRefCount() > 1)
        {
            n_warning("ModuleManager: module '%s' still has external references on unload (%d), keeping shared library loaded\n", loaded.config.name.AsCharPtr(), loaded.feature->GetRefCount());
            return false;
        }

        loaded.feature = nullptr;
    }

    if (loaded.library != nullptr)
    {
        if (loaded.library->IsLoaded())
        {
            loaded.library->Close();
        }
        delete loaded.library;
        loaded.library = nullptr;
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
ModuleManager::ResolveLibraryPath(const RuntimeModuleConfig& moduleConfig) const
{
    if (moduleConfig.path.IsValid())
        return moduleConfig.path;

    if (!moduleConfig.name.IsValid())
        return "";

    Util::String path = moduleConfig.name;

    if (path.ContainsCharFromSet("/\\"))
        return path;

#if __WIN32__
    if (!path.EndsWithString(".dll"))
    {
        path.Append(".dll");
    }
#else
    if (!path.BeginsWithString("lib"))
    {
        path = Util::String::Sprintf("lib%s", path.AsCharPtr());
    }
    if (!path.EndsWithString(".so"))
    {
        path.Append(".so");
    }
#endif

    if (IO::FSWrapper::FileExists(path))
    {
        return IO::IoServer::NativePath(path);
    }

#if defined(NEBULA_BINARY_FOLDER)
    Util::String deployRoot = NEBULA_BINARY_FOLDER;
    deployRoot.ConvertBackslashes();
    Util::String deployCandidate = Util::String::AppendPath(deployRoot, path);
    if (IO::FSWrapper::FileExists(deployCandidate))
    {
        return deployCandidate;
    }
#endif

    return path;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ModuleManager::FindLoadedModuleIndex(const Util::String& moduleName) const
{
    Util::String checkName = moduleName;
    checkName.ToLower();

    for (IndexT i = 0; i < this->loadedModules.Size(); i++)
    {
        Util::String loadedName = this->loadedModules[i].config.name;
        loadedName.ToLower();
        if (loadedName == checkName)
            return i;
    }

    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
ModuleManager::QueueModuleReload(const Util::String& moduleName)
{
    // Deduplicate: only queue a reload once per frame
    if (this->pendingReloads.FindIndex(moduleName) != InvalidIndex)
        return;

    n_printf("ModuleManager: reload of '%s' queued for next frame boundary\n", moduleName.AsCharPtr());
    this->pendingReloads.Append(moduleName);
}

//------------------------------------------------------------------------------
/**
*/
void
ModuleManager::ProcessPendingReloads(GameServer* gameServer)
{
    if (this->pendingReloads.IsEmpty())
        return;

    // Snapshot and clear the queue before processing so that reloads triggered
    // during the reload itself are deferred to the following frame.
    Util::Array<Util::String> toProcess = this->pendingReloads;
    this->pendingReloads.Clear();

    for (IndexT i = 0; i < toProcess.Size(); i++)
    {
        this->ReloadModuleByName(toProcess[i], gameServer);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::ReloadModuleByName(const Util::String& moduleName, GameServer* gameServer)
{
    IndexT idx = this->FindLoadedModuleIndex(moduleName);

    if (idx == InvalidIndex)
    {
        n_warning("ModuleManager: reload of '%s' failed: module is not loaded\n", moduleName.AsCharPtr());
        return false;
    }

    // Save config so we can re-load with the same settings
    RuntimeModuleConfig config = this->loadedModules[idx].config;

    n_printf("ModuleManager: reloading '%s'...\n", moduleName.AsCharPtr());

    if (!this->UnloadModule(this->loadedModules[idx], gameServer))
    {
        n_warning("ModuleManager: reload of '%s' blocked: module could not be safely unloaded\n", moduleName.AsCharPtr());
        return false;
    }

    this->loadedModules.EraseIndex(idx);

    if (!this->LoadModule(config, gameServer, false))
    {
        n_warning("ModuleManager: reload of '%s' failed during load\n", moduleName.AsCharPtr());
        return false;
    }

    n_printf("ModuleManager: reload of '%s' complete\n", moduleName.AsCharPtr());
    return true;
}

} // namespace Game
