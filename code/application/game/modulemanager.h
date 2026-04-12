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

private:
    struct LoadedModule;

    bool LoadModule(const RuntimeModuleConfig& moduleConfig, GameServer* gameServer, bool strictMode);
    void UnloadModule(LoadedModule& loaded, GameServer* gameServer);
    Util::String ResolveLibraryPath(const RuntimeModuleConfig& moduleConfig) const;

    Util::Array<LoadedModule> loadedModules;
};

} // namespace Game
//------------------------------------------------------------------------------
