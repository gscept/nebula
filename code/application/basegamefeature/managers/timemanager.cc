//------------------------------------------------------------------------------
//  timemanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "timemanager.h"
#include "timing/time.h"
#include "profiling/profiling.h"

using namespace Timing;
using namespace Core;

namespace Game
{

__ImplementClass(Game::TimeManager, 'TiMa', Game::Manager);
__ImplementSingleton(Game::TimeManager)

//------------------------------------------------------------------------------
/**
*/
TimeManager::TimeManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
TimeManager::~TimeManager()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::OnActivate()
{
    Manager::OnActivate();

    Time::Setup();

	if (!FrameSync::FrameSyncTimer::HasInstance())
	{
		this->frameSyncTimer = FrameSync::FrameSyncTimer::Create();
		this->frameSyncTimerOwner = true;
		this->frameSyncTimer->StartTime();
	}
	else
	{
		this->frameSyncTimer = FrameSync::FrameSyncTimer::Instance();
	}

    // register default time sources

    TimeSourceCreateInfo systemTimeInfo;
    systemTimeInfo.hash = TIMESOURCE_SYSTEM;
    Time::CreateTimeSource(systemTimeInfo);

    TimeSourceCreateInfo gameTimeInfo;
    gameTimeInfo.hash = TIMESOURCE_GAMEPLAY;
    Time::CreateTimeSource(gameTimeInfo);

    TimeSourceCreateInfo inputTimeInfo;
    inputTimeInfo.hash = TIMESOURCE_INPUT;
    Time::CreateTimeSource(inputTimeInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::OnDeactivate()
{
    if (this->frameSyncTimerOwner)
    {
        this->frameSyncTimer = nullptr;
        this->frameSyncTimerOwner = false;
    }

    Time::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::OnBeginFrame()
{
    N_MARKER_BEGIN(TimeManagerOnBeginFrame, Game);

    if (this->frameSyncTimerOwner)
        this->frameSyncTimer->UpdateTimePolling();

    Time::BeginFrame(this->frameSyncTimer->GetFrameTicks());

    N_MARKER_END();
}

} // namespace Game
