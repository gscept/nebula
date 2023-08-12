//------------------------------------------------------------------------------
//  timemanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "timemanager.h"
#include "timing/time.h"
#include "framesync/framesynctimer.h"

using namespace Timing;
using namespace Core;

namespace Game
{

namespace TimeManager
{
    //------------------------------------------------------------------------------
    /**
        This struct needs to be structured exactly like the TimeSource struct, just
        with different qualifiers.
    */
    struct TimeSourceState
    {
        Timing::Time frameTime = 0.0f;
        Timing::Time time = 0.0f;
        Timing::Tick ticks = 0;
        int pauseCounter = 0;
        float timeFactor = 1.0f;
    };

    static_assert(sizeof(TimeSource) == sizeof(TimeSourceState));
    static_assert(alignof(TimeSource) == alignof(TimeSourceState));

    /// timemanager singleton state
    struct State
    {
        /// global time factor
        float timeFactor = 1.0f;
        /// global ticks
        Timing::Tick time = 0.0f;
        /// global delta frame time
        Timing::Tick frameTime = 167; // start with dt = 1/60s

        TimeSourceState timeSources[32];
        uint numTimeSources = 0;

        Util::HashTable<uint32_t, uint32_t, 32, 1> timeSourceTable;

		Ptr<FrameSync::FrameSyncTimer> frameSyncTimer;
		bool frameSyncTimerOwner = false;
    };

    static State* state = nullptr;

    /// Destroy the singleton
    void Destroy();
    /// called after create
    void OnActivate();
    /// called at beginning of each frame
    void OnBeginFrame();
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
TimeManager::Create()
{
    n_assert(state == nullptr);
    state = n_new(State);

    Game::ManagerAPI api;
    api.OnActivate = &OnActivate;
    api.OnDeactivate = &Destroy;
    api.OnBeginFrame = &OnBeginFrame;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
TimeSource* const
TimeManager::CreateTimeSource(TimeSourceCreateInfo const& info)
{
    state->timeSourceTable.Add(info.hash, state->numTimeSources);
    n_assert(state->numTimeSources + 1 < 32);
    TimeSourceState& timesource = state->timeSources[state->numTimeSources++];
    timesource.frameTime = Timing::TicksToSeconds(state->frameTime);
    return reinterpret_cast<TimeSource*>(&timesource);
}

//------------------------------------------------------------------------------
/**
*/
TimeSource* const
TimeManager::GetTimeSource(uint32_t TIMESOURCE_HASH)
{
    return reinterpret_cast<TimeSource*>(&state->timeSources[state->timeSourceTable[TIMESOURCE_HASH]]);
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::SetGlobalTimeFactor(float factor)
{
    state->timeFactor = factor;
}

//------------------------------------------------------------------------------
/**
*/
float
TimeManager::GetGlobalTimeFactor()
{
    return state->timeFactor;
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::Destroy()
{
    n_assert(state != nullptr);
    n_delete(state);
    state = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::OnActivate()
{
	if (!FrameSync::FrameSyncTimer::HasInstance())
	{
		state->frameSyncTimer = FrameSync::FrameSyncTimer::Create();
		state->frameSyncTimerOwner = true;
		state->frameSyncTimer->StartTime();
	}
	else
	{
		state->frameSyncTimer = FrameSync::FrameSyncTimer::Instance();
	}

    //state->time = FrameSync::FrameSyncTimer::Instance()->GetTicks();

    // register default time sources

    TimeSourceCreateInfo systemTimeInfo;
    systemTimeInfo.hash = TIMESOURCE_SYSTEM;
    CreateTimeSource(systemTimeInfo);

    TimeSourceCreateInfo gameTimeInfo;
    gameTimeInfo.hash = TIMESOURCE_GAMEPLAY;
    CreateTimeSource(gameTimeInfo);

    TimeSourceCreateInfo inputTimeInfo;
    inputTimeInfo.hash = TIMESOURCE_INPUT;
    CreateTimeSource(inputTimeInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::OnBeginFrame()
{
	if (state->frameSyncTimerOwner)
		state->frameSyncTimer->UpdateTimePolling();

    // compute the current frame time
    Tick const realTimeFrameTicks = FrameSync::FrameSyncTimer::Instance()->GetFrameTicks();
    state->frameTime = (Timing::Tick)((float)realTimeFrameTicks * state->timeFactor);
    state->time = state->time + state->frameTime;

    // update all time sources
    Timing::Time const frameTimeInSecs = Timing::TicksToSeconds(state->frameTime);
    IndexT i;
    for (i = 0; i < state->numTimeSources; i++)
    {
        if (state->timeSources[i].pauseCounter == 0)
        {
            state->timeSources[i].frameTime = frameTimeInSecs * state->timeSources[i].timeFactor;
            state->timeSources[i].ticks += (Timing::Tick)((float)state->frameTime * state->timeSources[i].timeFactor);
            state->timeSources[i].time = Timing::TicksToSeconds(state->timeSources[i].ticks);
        }
        else
        {
            state->timeSources[i].frameTime = 0;
        }
    }
}

} // namespace Game
