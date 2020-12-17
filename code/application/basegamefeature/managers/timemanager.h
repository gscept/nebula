#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::TransformManager

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
*/
struct TimeSourceCreateInfo
{
    uint32_t fourcc;
};

//------------------------------------------------------------------------------
/**
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
*/
namespace TimeManager
{
    /// Create the singleton
    Game::ManagerAPI Create();

    /// register a timesource
    TimeSource* RegisterTimeSource(TimeSourceCreateInfo const& info);

    /// get a time source by hash
    TimeSource* GetTimeSource(uint32_t TIMESOURCE_HASH);

    /// set global time scale. This should be used sparingly. You can usually set individual time sources time factor instead.
    void SetGlobalTimeFactor(float factor);

    /// get the global time scale
    float GetGlobalTimeFactor();
};

} // namespace Game
