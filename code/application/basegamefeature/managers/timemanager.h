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
#include "game/timesource.h"
#include "framesync/framesynctimer.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
    @class Game::TimeManager

    Singleton object which manages all Game::TimeSource objects. The time sources themselves are in
    game/timesource.h, this is the per-frame driver which feeds them the real frame time.
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

private:
    Ptr<FrameSync::FrameSyncTimer> frameSyncTimer;
    bool frameSyncTimerOwner = false;
};

} // namespace Game
