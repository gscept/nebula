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
#include "coregraphics/swapchain.h"

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
            CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(CoreGraphics::MainWindow);
            CoreGraphics::SwapchainId swapchain = CoreGraphics::WindowGetSwapchain(CoreGraphics::MainWindow);

            CoreGraphics::SwapchainSwap(swapchain);
            CoreGraphics::QueueType queue = CoreGraphics::SwapchainGetQueueType(swapchain);

            // Allocate command buffer to run swap
            CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::SwapchainAllocateCmds(swapchain);
            CoreGraphics::CmdBufferBeginInfo beginInfo;
            beginInfo.submitDuringPass = false;
            beginInfo.resubmittable = false;
            beginInfo.submitOnce = true;
            CoreGraphics::CmdBeginRecord(cmdBuf, beginInfo);
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TURQOISE, "Swap");

            FrameScript_default::Synchronize("Present_Sync", cmdBuf, CoreGraphics::GraphicsQueueType, { { (FrameScript_default::TextureIndex)FrameScript_default::Export_ColorBuffer.index, CoreGraphics::PipelineStage::TransferRead } }, nullptr);
            CoreGraphics::SwapchainCopy(swapchain, cmdBuf, FrameScript_default::Export_ColorBuffer.tex);

            CoreGraphics::CmdEndMarker(cmdBuf);
            CoreGraphics::CmdFinishQueries(cmdBuf);
            CoreGraphics::CmdEndRecord(cmdBuf);
            auto submission = CoreGraphics::SubmitCommandBuffers(
                { cmdBuf }
                , queue
                , { FrameScript_default::Submission_Scene }
#if NEBULA_GRAPHICS_DEBUG
                , "Swap"
#endif

            );
            CoreGraphics::DeferredDestroyCmdBuffer(cmdBuf);
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
