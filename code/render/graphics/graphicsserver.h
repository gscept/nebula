#pragma once
//------------------------------------------------------------------------------
/**
    @class Graphics::GraphicsServer

    The graphics server is the main singleton for the Graphics subsystem.

    Updating the GraphicsServer will progress the rendering process by one frame. 
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "framesync/framesynctimer.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"
#include "graphicsentity.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shaperenderer.h"
#include "coregraphics/textrenderer.h"
#include "graphics/view.h"
#include "debug/debughandler.h"

namespace Graphics
{

struct FrameContext
{
    Timing::Time time;
    Timing::Tick ticks;
    Timing::Time frameTime;
    IndexT frameIndex;
    IndexT bufferIndex;
};

using ViewIndependentCall = void(*)(const Graphics::FrameContext& ctx);
using ViewDependentCall = void(*)(const ViewId view, const Graphics::FrameContext& ctx);


class GraphicsContext;
struct GraphicsContextFunctionBundle;
struct GraphicsContextState;
struct ViewId;
class GraphicsServer : public Core::RefCounted
{
    __DeclareClass(GraphicsServer);
    __DeclareSingleton(GraphicsServer);
public:
    /// constructor
    GraphicsServer();
    /// destructor
    virtual ~GraphicsServer();

    /// opens the graphics server
    void Open();
    /// closes the graphics server
    void Close();

    /// create graphics entity
    GraphicsEntityId CreateGraphicsEntity();
    /// discard graphics entity
    void DiscardGraphicsEntity(const GraphicsEntityId id);
    /// check if graphics entity is valid
    bool IsValidGraphicsEntity(const GraphicsEntityId id);

    /// create a new view with a new framescript
    Graphics::ViewId CreateView(
        const Util::StringAtom& name
        , bool(*renderFunction)(const Math::rectangle<int>&, IndexT, IndexT)
        , const Math::rectangle<int>& viewport
        , Graphics::StageMask stageMask = PRIMARY_STAGE_MASK
        , std::function<void(IndexT, IndexT)> preViewCallback = nullptr
        , std::function<void(IndexT, IndexT)> postViewCallback = nullptr
    );
    /// create a new view without a framescript
    Graphics::ViewId CreateView(const Util::StringAtom& name);
    /// Get view by name
    const Graphics::ViewId GetView(const Util::StringAtom& name);
    /// discard view
    void DiscardView(const Graphics::ViewId view);
    /// get current view
    const Graphics::ViewId GetCurrentView() const;
    /// set current view (do not use unless you know what you are doing since this is normally handled by the graphicssserver)
    void SetCurrentView(const Graphics::ViewId view);

    /// Add callback to run just before frame is finished
    void AddEndFrameCall(void(*func)(IndexT frameIndex, IndexT bufferIndex));
    /// Set a function to be run when resize
    void SetResizeCall(void(*)(const SizeT, const SizeT));

    /// Add window to update
    void AddWindow(const CoreGraphics::WindowId window);
    /// Remove window
    void RemoveWindow(const CoreGraphics::WindowId window);
    /// Get windows
    const Util::Array<CoreGraphics::WindowId>& GetWindows() const;

    /// Setup pre game logic graphics calls
    void SetupPreLogicCalls(const Util::Array<ViewIndependentCall>& calls);
    /// Setup post game logic graphics calls
    void SetupPostLogicCalls(const Util::Array<ViewIndependentCall>& calls);
    /// Setup per-view calls
    void SetupPreLogicViewCalls(const Util::Array<ViewDependentCall>& calls);
    /// Setup per-view calls
    void SetupPostLogicViewCalls(const Util::Array<ViewDependentCall>& calls);

    /// Run pre-logic calls
    void RunPreLogic();
    /// Run post-logic calls
    void RunPostLogic();
    /// Render views
    void Render();

    /// End the frame and submit
    void EndFrame();
    /// Progress to next frame
    void NewFrame();

    /// get total time in seconds
    const Timing::Time GetTime() const;
    /// get frame time in seconds
    const Timing::Time GetFrameTime() const;
    /// get frame index
    const IndexT GetFrameIndex() const;

    /// debug rendering
    void RenderDebug(uint32_t flags);

    /// register function bundle from graphics context, see GraphicsContextType::Create
    void RegisterGraphicsContext(GraphicsContextFunctionBundle* context, GraphicsContextState* state);
    /// unregister function bundle
    void UnregisterGraphicsContext(GraphicsContextFunctionBundle* context);
    
    /// call when the window has been resized
    bool OnWindowResized(CoreGraphics::WindowId wndId);

private:

    Ids::IdGenerationPool entityPool;

    Ptr<FrameSync::FrameSyncTimer> timer;
    bool ownsTimer;
    FrameContext frameContext;

    Util::Array<GraphicsContextFunctionBundle*> contexts;
    Util::Array<GraphicsContextState*> states;

    Util::Dictionary<Util::StringAtom, Graphics::ViewId> viewsByName;
    Util::Array<Graphics::ViewId> views;
    Graphics::ViewId currentView;

    Util::Array<std::function<void(IndexT, IndexT)>> preViewCallbacks;
    Util::Array<std::function<void(IndexT, IndexT)>> postViewCallbacks;
    Util::Array<std::function<void(IndexT, IndexT)>> endFrameCallbacks;

    void (*resizeCall) (const SizeT, const SizeT);

    Ptr<CoreGraphics::DisplayDevice> displayDevice;
    bool graphicsDevice;
    Ptr<CoreGraphics::ShaderServer> shaderServer;
    Ptr<CoreGraphics::ShapeRenderer> shapeRenderer;
    Ptr<CoreGraphics::TextRenderer> textRenderer;

    Util::Array<ViewIndependentCall> preLogicCalls, postLogicCalls;
    Util::Array<ViewDependentCall> preLogicViewCalls, postLogicViewCalls;

    Util::Array<CoreGraphics::WindowId> windows;
    SizeT maxWindowWidth, maxWindowHeight;

    bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
static GraphicsEntityId
CreateEntity()
{
    return GraphicsServer::Instance()->CreateGraphicsEntity();
}

//------------------------------------------------------------------------------
/**
*/
static void
DestroyEntity(const GraphicsEntityId id)
{
    GraphicsServer::Instance()->DiscardGraphicsEntity(id);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... CONTEXTS>
static void
RegisterEntity(const GraphicsEntityId id)
{
    (CONTEXTS::RegisterEntity(id), ...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... CONTEXTS>
static void
DeregisterEntity(const GraphicsEntityId id)
{
    (CONTEXTS::DeregisterEntity(id), ...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename ... CONTEXTS>
static void
DeregisterEntityImmediate(const GraphicsEntityId id)
{
    (CONTEXTS::DeregisterEntityImmediate(id), ...);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
GraphicsServer::SetResizeCall(void(*func)(const SizeT windowWidth, const SizeT windowHeight))
{
    this->resizeCall = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GraphicsServer::SetupPreLogicCalls(const Util::Array<ViewIndependentCall>& calls)
{
    this->preLogicCalls = calls;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GraphicsServer::SetupPostLogicCalls(const Util::Array<ViewIndependentCall>& calls)
{
    this->postLogicCalls = calls;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GraphicsServer::SetupPreLogicViewCalls(const Util::Array<ViewDependentCall>& calls)
{
    this->preLogicViewCalls = calls;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GraphicsServer::SetupPostLogicViewCalls(const Util::Array<ViewDependentCall>& calls)
{
    this->postLogicViewCalls = calls;
}

//------------------------------------------------------------------------------
/**
*/
inline const Timing::Time 
GraphicsServer::GetTime() const
{
    return this->frameContext.time;
}

//------------------------------------------------------------------------------
/**
*/
inline const Timing::Time
GraphicsServer::GetFrameTime() const
{
    return this->frameContext.frameTime;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT
GraphicsServer::GetFrameIndex() const
{
    return this->frameContext.frameIndex;
}



} // namespace Graphics
