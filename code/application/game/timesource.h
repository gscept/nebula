#pragma once
//------------------------------------------------------------------------------
/**
    @file timesource.h

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "timing/time.h"

#define TIMESOURCE_GAMEPLAY uint32_t('GPTS')
#define TIMESOURCE_INPUT uint32_t('IPTS')
#define TIMESOURCE_SYSTEM uint32_t('SYTS')

namespace Game
{

//------------------------------------------------------------------------------
/**
    Used to create a timesource.
*/
struct TimeSourceCreateInfo
{
    /// time source hash number. Ex. 'ABC1'
    uint32_t hash = 0;
    /// when non-zero the time source advances in fixed steps instead of following real frame time
    Timing::Time fixedFrameTime = 0.0;
    /// how many fixed steps the source can accumulate per frame. Guards against the spiral of death
    IndexT maxStepsPerFrame = 5;
};

//------------------------------------------------------------------------------
/**
    @struct Game::TimeSource

    @brief An object that keeps track of running time and frame time (delta time).
    
    @details Each time source tracks its own time independently from the other
    time sources, they can also be paused and unpaused independentlty 
    from each other, and they may also run faster or slower then
    realtime. 

    A time source created with a fixed frame time does not follow realtime. It accumulates real time and turns it
    into whole steps of fixedFrameTime, and frameTime is then always that step. The steps are consumed by
    Game::Time::ConsumeFixedStep, which is what a fixed frame event in the frame pipeline runs on.

    @note You should never create TimeSources by using `new`.
    Instead, use Game::Time::CreateTimeSource.

    @todo These are currently not thread safe.

    @see Game::Time::CreateTimeSource
    @see Game::Time::GetTimeSource
    @see Game::Time::ConsumeFixedStep
*/
struct TimeSource
{
    /// delta time between this frame and last frame. Always fixedFrameTime for a fixed time source
    Timing::Time const frameTime;
    /// total time this timesource has run for
    Timing::Time const time;
    /// number of ticks that this time source has run for
    Timing::Tick const ticks;
    /// the fixed step this source advances by, 0 when the source follows real frame time
    Timing::Time const fixedFrameTime;
    /// real time which has not been turned into a fixed step yet
    Timing::Time const accumulator;
    /// fixed steps available to run this frame
    IndexT const availableSteps;
    /// max fixed steps available per frame
    IndexT const maxStepsPerFrame;
    /// increment this to pause time, decrement to continue running
    int pauseCounter;
    /// time factor for this time source
    float timeFactor;
};

//------------------------------------------------------------------------------
/**
    @namespace Game::Time

    These are the standard time source objects provided by Application layer:

    @item TIMESOURCE_SYSTEM   - timing for low level Application layer subsystems
    @item TIMESOURCE_GAMEPLAY - timing for the game logic
    @item TIMESOURCE_INPUT    - extra time source for input handling
*/
namespace Time
{
    /// create a timesource. The global time manager handles the timesources.
    TimeSource* const CreateTimeSource(TimeSourceCreateInfo const& info);

    /// get a time source by hash
    TimeSource* const GetTimeSource(uint32_t TIMESOURCE_HASH);

    /// set global time scale. This should be used sparingly. You can usually set individual time sources time factor instead.
    void SetGlobalTimeFactor(float factor);

    /// get the global time scale
    float GetGlobalTimeFactor();

    /// setup the time sources. Called by Game::TimeManager on activate
    void Setup();

    /// discard the time sources. Called by Game::TimeManager on deactivate
    void Discard();

    /// advance every time source by the real frame time. A fixed source turns real time into available steps
    void BeginFrame(Timing::Tick realFrameTicks);

    /// consume one fixed step from a fixed time source. Returns false when no steps are left
    bool ConsumeFixedStep(TimeSource* timeSource);

    /// add time to the accumulator of a fixed time source. For tests and tools which drive the rate explicitly.
    /// Warns in debug when the source's maxStepsPerFrame clamps away part of what was asked for
    void AdvanceTimeSource(TimeSource* timeSource, Timing::Time delta);
};

} // namespace Game
