//------------------------------------------------------------------------------
//  timemanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "timemanager.h"
#include "timing/time.h"
#include "framesync/framesynctimer.h"
#include "profiling/profiling.h"

using namespace Timing;
using namespace Core;

namespace Game
{

namespace Time
{
    //------------------------------------------------------------------------------
    /**
        \cond EXCLUDE_FROM_DOCS

        IMPORTANT: This struct needs to be structured exactly like the
        TimeSource struct, just with different qualifiers.
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

    //------------------------------------------------------------------------------
    /**
        timemanager singleton state
    */
    
    struct State
    {
        /// global time factor
        float timeFactor = 1.0f;
        /// global ticks
        Timing::Tick time = 0.0f;
        /// global delta frame time
        Timing::Tick frameTime = 167; // start with dt = 1/60s

        TimeSourceState timeSources[32];
        uint32_t numTimeSources = 0;

        Util::HashTable<uint32_t, uint32_t, 32, 1> timeSourceTable;

		Ptr<FrameSync::FrameSyncTimer> frameSyncTimer;
		bool frameSyncTimerOwner = false;
    };

    static State* state = nullptr;

    /// \endcond
}

//------------------------------------------------------------------------------
/**
*/
TimeSource* const
Time::CreateTimeSource(TimeSourceCreateInfo const& info)
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
bool
Time::HasTimeSource(uint32_t TIMESOURCE_HASH)
{
    return state->timeSourceTable.Contains(TIMESOURCE_HASH);
}

//------------------------------------------------------------------------------
/**
*/
void
Time::DestroyTimeSource(uint32_t TIMESOURCE_HASH)
{
    if (!state->timeSourceTable.Contains(TIMESOURCE_HASH))
        return;

    const uint32_t removeIndex = state->timeSourceTable[TIMESOURCE_HASH];
    const uint32_t lastIndex = state->numTimeSources - 1;

    // Remove hash entry first.
    state->timeSourceTable.Erase(TIMESOURCE_HASH);

    // Keep array dense by moving the last entry into the removed slot.
    if (removeIndex != lastIndex)
    {
        state->timeSources[removeIndex] = state->timeSources[lastIndex];

        // Find hash currently mapped to lastIndex and update it to removeIndex.
        auto content = state->timeSourceTable.Content();
        for (IndexT i = 0; i < content.Size(); i++)
        {
            if (content[i].Value() == lastIndex)
            {
                state->timeSourceTable[content[i].Key()] = removeIndex;
                break;
            }
        }
    }

    n_assert(state->numTimeSources > 0);
    state->numTimeSources--;
}

//------------------------------------------------------------------------------
/**
*/
TimeSource* const
Time::GetTimeSource(uint32_t TIMESOURCE_HASH)
{
    return reinterpret_cast<TimeSource*>(&state->timeSources[state->timeSourceTable[TIMESOURCE_HASH]]);
}

//------------------------------------------------------------------------------
/**
*/
void
Time::SetGlobalTimeFactor(float factor)
{
    state->timeFactor = factor;
}

//------------------------------------------------------------------------------
/**
*/
float
Time::GetGlobalTimeFactor()
{
    return state->timeFactor;
}

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

    n_assert(Time::state == nullptr);
    Time::state = new Time::State;

	if (!FrameSync::FrameSyncTimer::HasInstance())
	{
		Time::state->frameSyncTimer = FrameSync::FrameSyncTimer::Create();
		Time::state->frameSyncTimerOwner = true;
		Time::state->frameSyncTimer->StartTime();
	}
	else
	{
		Time::state->frameSyncTimer = FrameSync::FrameSyncTimer::Instance();
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
    n_assert(Time::state != nullptr);
    delete Time::state;
    Time::state = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
TimeManager::OnBeginFrame()
{
    N_MARKER_BEGIN(TimeManagerOnBeginFrame, Game);
    using namespace Time;

	if (state->frameSyncTimerOwner)
		state->frameSyncTimer->UpdateTimePolling();

    // compute the current frame time
    Tick const realTimeFrameTicks = FrameSync::FrameSyncTimer::Instance()->GetFrameTicks();
    state->frameTime = (Timing::Tick)((float)realTimeFrameTicks * state->timeFactor);
    state->time = state->time + state->frameTime;

    // update all time sources
    Timing::Time const frameTimeInSecs = Timing::TicksToSeconds(state->frameTime);
    for (uint32_t i = 0; i < state->numTimeSources; i++)
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
    N_MARKER_END();
}

} // namespace Game
