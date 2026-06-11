//------------------------------------------------------------------------------
//  runtimemoduletest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "runtimemoduletest.h"
#include "game/featureunit.h"
#include "game/modulemanager.h"
#include "game/gameserver.h"

namespace Test
{
__ImplementClass(Test::RuntimeModuleLoaderTest, 'RMLT', Test::TestCase);

void
RuntimeModuleLoaderTest::Run()
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
        Game::RuntimeModuleConfig missing;
        missing.name = "does_not_exist";
        missing.enabled = true;
        missing.required = false;
        configs.Append(missing);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig missing;
        missing.name = "does_not_exist";
        missing.enabled = true;
        missing.required = true;
        configs.Append(missing);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(!ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig badExport;
        badExport.name = "testruntimemodulebadexports";
        badExport.enabled = true;
        badExport.required = false;
        configs.Append(badExport);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    {
        Util::Array<Game::RuntimeModuleConfig> configs;
        Game::RuntimeModuleConfig badAbi;
        badAbi.name = "testruntimemodulebadabi";
        badAbi.enabled = true;
        badAbi.required = false;
        configs.Append(badAbi);

        bool ok = manager->LoadModules(configs, server, false);
        VERIFY(ok);
        VERIFY(manager->GetNumLoadedModules() == 0);
    }

    manager = nullptr;
}

} // namespace Test
