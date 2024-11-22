#pragma once
//------------------------------------------------------------------------------
/**
    @class  Tests::GameStateManager

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "renderutil/mouserayutil.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "graphics/cameracontext.h"
#include "models/nodes/primitivenode.h"
#include "models/modelcontext.h"
#include "imgui.h"
#include "dynui/im3d/im3d.h"
#include "dynui/im3d/im3dcontext.h"
#include "timing/timer.h"

namespace Tests
{

class GameStateManager : public Game::Manager
{
    __DeclareClass(GameStateManager)
    __DeclareSingleton(GameStateManager)
public:
    GameStateManager();
    virtual ~GameStateManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
    void OnFrame() override;
};

} // namespace Demo
