//------------------------------------------------------------------------------
//  gameserver.cc
//  (C) 2003 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "game/gameserver.h"
#include "core/factory.h"

namespace Game
{
__ImplementClass(Game::GameServer, 'GMSV', Core::RefCounted);
__ImplementSingleton(Game::GameServer);

//------------------------------------------------------------------------------
/**
*/
GameServer::GameServer() :
    isOpen(false),
    isStarted(false)
{
    __ConstructSingleton;
	_setup_grouped_timer(GameServerOnBeginFrame, "Game Subsystem");
	_setup_grouped_timer(GameServerOnFrame, "Game Subsystem");
	_setup_grouped_timer(GameServerOnEndFrame, "Game Subsystem");
}

//------------------------------------------------------------------------------
/**
*/
GameServer::~GameServer()
{
    n_assert(!this->isOpen);
	_discard_timer(GameServerOnBeginFrame);
    _discard_timer(GameServerOnFrame);
	_discard_timer(GameServerOnEndFrame);
	
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Initialize the game server object. This will create and initialize all
    subsystems.
*/
bool
GameServer::Open()
{
    n_assert(!this->isOpen);
    n_assert(!this->isStarted);
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the game server object.
*/
void
GameServer::Close()
{
    n_assert(!this->isStarted);
    n_assert(this->isOpen);
    
    // remove all gameFeatures
    while (this->gameFeatures.Size() > 0)
    {
        this->gameFeatures[0]->OnDeactivate();
        this->gameFeatures.EraseIndex(0);
    }
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::AttachGameFeature(const Ptr<FeatureUnit>& feature)
{
    n_assert(0 != feature);
    n_assert(InvalidIndex == this->gameFeatures.FindIndex(feature));
    feature->OnActivate();
    this->gameFeatures.Append(feature);
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::RemoveGameFeature(const Ptr<FeatureUnit>& feature)
{
    n_assert(0 != feature);
    IndexT index = this->gameFeatures.FindIndex(feature);
    n_assert(InvalidIndex != index);
    feature->OnDeactivate();
    this->gameFeatures.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
    Start the game world, called after loading has completed.
*/
bool
GameServer::Start()
{
    n_assert(this->isOpen);
    n_assert(!this->isStarted);

    // call the OnStart method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnStart();
    }
    
    this->isStarted = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
GameServer::HasStarted() const
{
    return this->isStarted;
}

//------------------------------------------------------------------------------
/**
    Stop the game world, called before the world(current level) is cleaned up.
*/
void
GameServer::Stop()
{
    n_assert(this->isOpen);
    n_assert(this->isStarted);
    
    this->isStarted = false;
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::OnBeginFrame()
{
	_start_timer(GameServerOnBeginFrame);

	// trigger game features to at the beginning of a frame
	IndexT i;
	SizeT num = this->gameFeatures.Size();
	for (i = 0; i < num; i++)
	{
		this->gameFeatures[i]->OnBeginFrame();
	}

	_stop_timer(GameServerOnBeginFrame);
}

//------------------------------------------------------------------------------
/**
    Trigger the game server. If your application introduces new or different
    manager objects, you may also want to override the Game::GameServer::Trigger()
    method if those gameFeatures need per-frame callbacks.
*/
void
GameServer::OnFrame()
{
    _start_timer(GameServerOnFrame);

    // call trigger functions on game features   
    IndexT i;
    SizeT num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnFrame();
    } 

    _stop_timer(GameServerOnFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::OnEndFrame()
{
	_start_timer(GameServerOnEndFrame);

	IndexT i;
	SizeT num = this->gameFeatures.Size();
	for (i = 0; i < num; i++)
	{
		this->gameFeatures[i]->OnEndFrame();
	}

	_stop_timer(GameServerOnEndFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::NotifyBeforeLoad()
{
	// call the SetupDefault method on all gameFeatures
	int i;
	int num = this->gameFeatures.Size();
	for (i = 0; i < num; i++)
	{
		this->gameFeatures[i]->OnBeforeLoad();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::NotifyBeforeCleanup()
{
	// call the SetupDefault method on all gameFeatures
	int i;
	int num = this->gameFeatures.Size();
	for (i = 0; i < num; i++)
	{
		this->gameFeatures[i]->OnBeforeCleanup();
	}
}

//------------------------------------------------------------------------------
/**    
*/
void
GameServer::NotifyGameLoad()
{
    // call the OnLoad method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnLoad();
    }
}

//------------------------------------------------------------------------------
/**    
*/
void
GameServer::NotifyGameSave()
{
    // call the OnLoad method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnSave();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
GameServer::IsFeatureAttached(const Util::String& stringName) const
{
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        if (this->gameFeatures[i]->GetRtti()->GetName() == stringName)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<FeatureUnit>> const&
GameServer::GetGameFeatures() const
{
	return this->gameFeatures;
}

} // namespace Game