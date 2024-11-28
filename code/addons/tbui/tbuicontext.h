#pragma once
//------------------------------------------------------------------------------
/**
    Turbobadger UI Context

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/shader.h"
#include "coregraphics/pipeline.h"
#include "tbuiinputhandler.h"
#include "threading/spinlock.h"

namespace TBUI
{
class TBUIRenderer;
class TBUIView;

class TBUIContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();

public:
    /// constructor
    TBUIContext();
    /// destructor
    virtual ~TBUIContext();

    /// Create context
    static void Create();
    /// Discard context
    static void Discard();

    static void FrameUpdate(const Graphics::FrameContext& ctx);

    static TBUIView* CreateView(int32_t width, int32_t height);

    static void DestroyView(const TBUIView* view);

    static bool ProcessInput(const Input::InputEvent& inputEvent);

    //static void LoadViewResource();

public:
    struct TBUIState
    {
        CoreGraphics::ShaderId shader;
        CoreGraphics::ShaderProgramId shaderProgram;
        CoreGraphics::PipelineId pipeline;
        Util::FixedArray<CoreGraphics::BufferId> vbos;
        CoreGraphics::VertexLayoutId vertexLayout;

        IndexT textProjectionConstant;
        IndexT textureConstant;
        CoreGraphics::ResourceTableId resourceTable;
        Util::FixedArray<byte*> vertexPtrs;

        Ptr<TBUIInputHandler> inputHandler;
    };

private:
    static void Render(
        const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex
    );

    /// called if the window size has changed
    static void OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);

private:
    static TBUIRenderer* renderer;
    static Util::Array<TBUIView*> views;
    static TBUIState state;
};

} // namespace TBUI
