//------------------------------------------------------------------------------
//  @file timesource.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game/timesource.h"
#include "util/hashtable.h"

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
        Timing::Time fixedFrameTime = 0.0f;
        Timing::Time accumulator = 0.0f;
        IndexT availableSteps = 0;
        IndexT maxStepsPerFrame = 5;
        int pauseCounter = 0;
        float timeFactor = 1.0f;
    };

    static_assert(sizeof(TimeSource) == sizeof(TimeSourceState));
    static_assert(alignof(TimeSource) == alignof(TimeSourceState));

    //------------------------------------------------------------------------------
    /**
        time source singleton state
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
    };

    static State* state = nullptr;

    //------------------------------------------------------------------------------
    /**
        Turn whatever real time a fixed source has accumulated into whole steps. The remainder stays in the
        accumulator, which is what makes a fixed source catch up when a frame runs long. Steps are added to what is
        already available, so a source can be advanced explicitly as well.
    */
    static void
    RefreshSteps(TimeSourceState& timesource)
    {
        if (timesource.fixedFrameTime <= 0.0)
            return;

        Timing::Time const maxAccumulator = timesource.fixedFrameTime * timesource.maxStepsPerFrame;
        if (timesource.accumulator > maxAccumulator)
            timesource.accumulator = maxAccumulator;

        while (timesource.accumulator >= timesource.fixedFrameTime)
        {
            timesource.accumulator -= timesource.fixedFrameTime;
            timesource.availableSteps++;
        }

        if (timesource.availableSteps > timesource.maxStepsPerFrame)
            timesource.availableSteps = timesource.maxStepsPerFrame;
    }

    /// \endcond
}

//------------------------------------------------------------------------------
/**
*/
void
Time::Setup()
{
    n_assert(state == nullptr);
    state = new State;
}

//------------------------------------------------------------------------------
/**
*/
void
Time::Discard()
{
    n_assert(state != nullptr);
    delete state;
    state = nullptr;
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
    timesource.fixedFrameTime = info.fixedFrameTime;
    timesource.maxStepsPerFrame = info.maxStepsPerFrame;
    return reinterpret_cast<TimeSource*>(&timesource);
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

//------------------------------------------------------------------------------
/**
*/
void
Time::BeginFrame(Timing::Tick realFrameTicks)
{
    state->frameTime = (Timing::Tick)((float)realFrameTicks * state->timeFactor);
    state->time = state->time + state->frameTime;

    // update all time sources
    Timing::Time const frameTimeInSecs = Timing::TicksToSeconds(state->frameTime);
    for (uint32_t i = 0; i < state->numTimeSources; i++)
    {
        TimeSourceState& timesource = state->timeSources[i];
        if (timesource.pauseCounter > 0)
        {
            timesource.frameTime = 0;
            timesource.availableSteps = 0;
            continue;
        }

        if (timesource.fixedFrameTime > 0.0)
        {
            timesource.accumulator += frameTimeInSecs * timesource.timeFactor;
            timesource.frameTime = timesource.fixedFrameTime;
            RefreshSteps(timesource);
        }
        else
        {
            timesource.frameTime = frameTimeInSecs * timesource.timeFactor;
            timesource.ticks += (Timing::Tick)((float)state->frameTime * timesource.timeFactor);
            timesource.time = Timing::TicksToSeconds(timesource.ticks);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Time::ConsumeFixedStep(TimeSource* timeSource)
{
    if (timeSource->availableSteps == 0)
        return false;

    // The public struct is a const view of the state, which is what the state struct mirrors
    TimeSourceState& timesource = *reinterpret_cast<TimeSourceState*>(timeSource);
    timesource.availableSteps--;
    timesource.time += timesource.fixedFrameTime;
    timesource.ticks = Timing::SecondsToTicks(timesource.time);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
Time::AdvanceTimeSource(TimeSource* timeSource, Timing::Time delta)
{
    TimeSourceState& timesource = *reinterpret_cast<TimeSourceState*>(timeSource);
    n_assert(timesource.fixedFrameTime > 0.0);
    timesource.accumulator += delta;
    RefreshSteps(timesource);
}

} // namespace Game
