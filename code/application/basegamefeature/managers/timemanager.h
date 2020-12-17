#pragma once
//------------------------------------------------------------------------------
/**
	Game::TimeManager

    Singleton object which manages the current game time. These are
    the standard time source objects provided by Application layer:

    TIMESOURCE_SYSTEM   - timing for low level Application layer subsystems
    TIMESOURCE_GAMEPLAY - timing for the game logic
    TIMESOURCE_INPUT    - extra time source for input handling
    
    Each time source tracks its own time independently from the other
    time sources, they can also be paused and unpaused independentlty 
    from each other, and they may also run faster or slower then
    realtime. 

    You can create custom time sources by using the create interface.

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "timing/time.h"

#define TIMESOURCE_GAMEPLAY uint32_t('GPTS')
#define TIMESOURCE_INPUT uint32_t('IPTS')
#define TIMESOURCE_SYSTEM uint32_t('1234')

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
};

//------------------------------------------------------------------------------
/**
    A generic time source POD struct which is created and deleted by the TimeManager.
    
    You can get TimeSources by calling the Game::TimeManager::GetTimeSource function.
*/
struct TimeSource
{
    /// delta time between this frame and last frame
    Timing::Time const frameTime;
    /// total time this timesource has run for
    Timing::Time const time;
    /// number of ticks that this time source has run for
    Timing::Tick const ticks;
    /// increment this to pause time, decrement to continue running
    int pauseCounter;
    /// time factor for this time source
    float timeFactor;
};

//------------------------------------------------------------------------------
/**
    @namespace TimeManager

    Interface to the TimeManager singleton.
*/
namespace TimeManager
{
    /// create the singleton
    Game::ManagerAPI Create();

    /// create a timesource. The time managers handles the timesources.
    TimeSource* const CreateTimeSource(TimeSourceCreateInfo const& info);

    /// get a time source by hash
    TimeSource* const GetTimeSource(uint32_t TIMESOURCE_HASH);

    /// set global time scale. This should be used sparingly. You can usually set individual time sources time factor instead.
    void SetGlobalTimeFactor(float factor);

    /// get the global time scale
    float GetGlobalTimeFactor();
};

} // namespace Game
