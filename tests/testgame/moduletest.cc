//------------------------------------------------------------------------------
//  moduletest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "moduletest.h"
#include "game/featureunit.h"
#include "game/modulemanager.h"
#include "game/gameserver.h"

namespace Test
{
__ImplementClass(Test::ModuleTest, 'GMDT', Test::TestCase);

void
ModuleTest::Run()
{
    VERIFY(Game::GameServer::HasInstance());
    Game::GameServer* server = Game::GameServer::Instance();
    const SizeT baseFeatureCount = server->GetGameFeatures().Size();

    Ptr<Game::ModuleManager> manager = Game::ModuleManager::Create();

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig cfg;
        cfg.name = "testruntimemodule";
        cfg.enabled = true;
        cfg.required = true;
        configs.Append(cfg);

        bool ok = manager->LoadModules(configs, server, true);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 1);
        VERIFY(manager->IsModuleLoaded("testruntimemodule"));
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount + 1);

        manager->UnloadModules(server);
        VERIFY(manager->GetNumLoadedModules() == 0);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;

        Game::RuntimeModuleConfig first;
        first.name = "testruntimemodule";
        first.enabled = true;
        first.required = true;
        configs.Append(first);

        Game::RuntimeModuleConfig duplicate = first;
        configs.Append(duplicate);

        bool ok = manager->LoadModules(configs, server, true);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 1);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount + 1);

        manager->UnloadModules(server);
        VERIFY(manager->GetNumLoadedModules() == 0);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig cfg;
        cfg.name = "testruntimemodule";
        cfg.enabled = true;
        cfg.required = true;
        configs.Append(cfg);

        bool ok = manager->LoadModules(configs, server, true);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 1);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount + 1);

        Ptr<Game::FeatureUnit> extraRef = server->GetGameFeatures().Back();
        VERIFY(extraRef.isvalid());

        manager->UnloadModules(server);
        VERIFY(manager->GetNumLoadedModules() == 0);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount);
        VERIFY(extraRef.isvalid());

        extraRef = nullptr;

        ok = manager->LoadModules(configs, server, true);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 1);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount + 1);

        manager->UnloadModules(server);
        VERIFY(manager->GetNumLoadedModules() == 0);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig cfg;
        cfg.name = "does_not_exist";
        cfg.enabled = true;
        cfg.required = false;
        configs.Append(cfg);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig cfg;
        cfg.name = "does_not_exist";
        cfg.enabled = true;
        cfg.required = true;
        configs.Append(cfg);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(!ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig cfg;
        cfg.name = "testruntimemodulebadexports";
        cfg.enabled = true;
        cfg.required = false;
        configs.Append(cfg);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig cfg;
        cfg.name = "testruntimemodulebadabi";
        cfg.enabled = true;
        cfg.required = false;
        configs.Append(cfg);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;

        Game::RuntimeModuleConfig missing;
        missing.name = "does_not_exist";
        missing.enabled = true;
        missing.required = false;
        configs.Append(missing);

        Game::RuntimeModuleConfig valid;
        valid.name = "testruntimemodule";
        valid.enabled = true;
        valid.required = true;
        configs.Append(valid);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 1);
        VERIFY(manager->IsModuleLoaded("testruntimemodule"));

        manager->UnloadModules(server);
        VERIFY(manager->GetNumLoadedModules() == 0);
        VERIFY(server->GetGameFeatures().Size() == baseFeatureCount);
    }

    manager = nullptr;
}

} // namespace Test
