//------------------------------------------------------------------------------
//  tbuifeatureunit.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "tbuifeatureunit.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "gamestatemanager.h"
#include "profiling/profiling.h"
#include "managers/inputmanager.h"
#include "game/api.h"
#include "game/world.h"
#include "appgame/gameapplication.h"
#include "frame/default.h"

namespace Tests
{

__ImplementClass(TBUIFeatureUnit, 'TBFU', Game::FeatureUnit);
__ImplementSingleton(TBUIFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
TBUIFeatureUnit::TBUIFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
TBUIFeatureUnit::~TBUIFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnAttach()
{
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    // Setup game state
    this->AttachManager(Tests::GameStateManager::Create());
    this->AttachManager(Tests::InputManager::Create());

#if WITH_NEBULA_EDITOR
    if (!App::GameApplication::IsEditorEnabled())
#endif
    {
        Graphics::GraphicsServer::Instance()->AddPostViewCall(
            [](IndexT frameIndex, IndexT bufferIndex)
            {
                Graphics::GraphicsServer::SwapInfo swapInfo;
                swapInfo.syncFunc = [](CoreGraphics::CmdBufferId cmdBuf)
                {
                    FrameScript_default::Synchronize(
                        "Present_Sync",
                        cmdBuf,
                        CoreGraphics::GraphicsQueueType,
                        {{(FrameScript_default::TextureIndex)FrameScript_default::Export_ColorBuffer.index,
                          CoreGraphics::PipelineStage::TransferRead}},
                        nullptr
                    );
                };
                swapInfo.submission = FrameScript_default::Submission_Scene;
                swapInfo.swapSource = FrameScript_default::Export_ColorBuffer.tex;
                Graphics::GraphicsServer::Instance()->SetSwapInfo(swapInfo);
            }
        );
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnBeginFrame()
{
    FeatureUnit::OnBeginFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();
}
} // namespace Tests
