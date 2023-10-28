//------------------------------------------------------------------------------
//  game/basegamefeature.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "core/factory.h"
#include "game/gameserver.h"
#include "io/ioserver.h"
#include "io/console.h"
#include "managers/blueprintmanager.h"
#include "managers/timemanager.h"
#include "imgui.h"
#include "basegamefeature/components/basegamefeature.h"

namespace BaseGameFeature
{
__ImplementClass(BaseGameFeature::BaseGameFeatureUnit, 'GAGF', Game::FeatureUnit);
__ImplementSingleton(BaseGameFeatureUnit);

using namespace App;
using namespace Game;

//------------------------------------------------------------------------------
/**
*/
BaseGameFeatureUnit::BaseGameFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
BaseGameFeatureUnit::~BaseGameFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnAttach()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    world->RegisterType<Game::Owner>();
    world->RegisterType<Game::Transform>();
    world->RegisterType<Game::IsActive>();
    world->RegisterType<Game::Static>();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    this->blueprintManager = this->AttachManager(BlueprintManager::Create());
    this->timeManager = this->AttachManager(TimeManager::Create());
    this->cl_debug_worlds = Core::CVarCreate(Core::CVar_Int, "cl_debug_worlds", "1", "Enable world debugging");
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnDeactivate()
{
    this->RemoveManager(this->blueprintManager);
    this->RemoveManager(this->timeManager);

    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnRenderDebug()
{
    //for (uint worldIndex = 0; worldIndex < numWorlds; worldIndex++)
    //    Game::World* world = Game::GameServer::Instance()->state.worlds[worldIndex];

    if (Core::CVarReadInt(this->cl_debug_worlds) == 0)
        return;
        
    ImGui::Begin("World inspector");
    {
        uint const numWorlds = Game::GameServer::Instance()->state.numWorlds;
        static int selectedWorld = 0;

		ImGui::InputInt("World index", &selectedWorld);

		selectedWorld = Math::clamp(selectedWorld, 0, 31);

		Game::World* world = Game::GameServer::Instance()->state.worlds[selectedWorld];

		if (world != nullptr)
        {
            //world->RenderDebug();
        }
        else
        {
            ImGui::TextDisabled("Invalid world index");
        }
    }
    ImGui::End();

    FeatureUnit::OnRenderDebug();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnFrame()
{
    FeatureUnit::OnFrame();
}

} // namespace Game
