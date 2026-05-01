#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::ModuleManager

    Loads FeatureUnit modules from shared libraries at startup.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/array.h"
#include "util/string.h"
#include "game/moduleinterface.h"

namespace Base
{
class Library;
}

namespace Game
{
class FeatureUnit;
class GameServer;

struct RuntimeModuleConfig
{
    Util::String name;
    Util::String path;
    bool enabled = true;
    bool required = false;
};

class ModuleManager : public Core::RefCounted
{
    __DeclareClass(ModuleManager)
public:
    ModuleManager();
    virtual ~ModuleManager();

    /// load all enabled modules and attach feature units to the game server
    bool LoadModules(const Util::Array<RuntimeModuleConfig>& moduleConfigs, GameServer* gameServer, bool strictMode = false);
    /// unload all modules and detach feature units from game server
    void UnloadModules(GameServer* gameServer);

    /// return true if a module with this name has been loaded
    bool IsModuleLoaded(const Util::String& moduleName) const;
    /// get number of loaded modules
    SizeT GetNumLoadedModules() const;

    /// queue a reload of the named module to be executed at the next frame boundary
    void QueueModuleReload(const Util::String& moduleName);
    /// process all pending module reloads; call once per frame from application layer after OnEndFrame
    void ProcessPendingReloads(GameServer* gameServer);

private:
    struct LoadedModule;

    bool LoadModule(const RuntimeModuleConfig& moduleConfig, GameServer* gameServer, bool strictMode);
    bool UnloadModule(LoadedModule& loaded, GameServer* gameServer);
    Util::String ResolveLibraryPath(const RuntimeModuleConfig& moduleConfig) const;
    IndexT FindLoadedModuleIndex(const Util::String& moduleName) const;
    /// unload then reload a single module by name; internal use by ProcessPendingReloads
    bool ReloadModuleByName(const Util::String& moduleName, GameServer* gameServer);

    Util::Array<LoadedModule> loadedModules;
    Util::Array<Util::String> pendingReloads;
};

} // namespace Game
//------------------------------------------------------------------------------
