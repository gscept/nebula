#pragma once
//------------------------------------------------------------------------------
/**
    @file timemanager.h

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "game/manager.h"
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
};

//------------------------------------------------------------------------------
/**
    @struct Game::TimeSource

    @brief An object that keeps track of running time and frame time (delta time).
    
    @details Each time source tracks its own time independently from the other
    time sources, they can also be paused and unpaused independentlty 
    from each other, and they may also run faster or slower then
    realtime. 

    @note You should never create TimeSources by using `new`.
    Instead, use Game::Time::CreateTimeSource.

    @todo These are currently not thread safe.

    @see Game::Time::CreateTimeSource
    @see Game::Time::GetTimeSource
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
    @namespace Time

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
};

//------------------------------------------------------------------------------
/**
    @class Game::TimeManager

    Singleton object which manages all Game::TimeSource objects.
*/
class TimeManager : public Game::Manager
{
    __DeclareClass(TimeManager)
    __DeclareSingleton(TimeManager)
public:
    TimeManager();
    virtual ~TimeManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
};

} // namespace Game
