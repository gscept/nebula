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
#include "tbuiinputhandler.h"

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

    //static void LoadViewResource();

private:
    static void Render(
        const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex
    );

    /// called if the window size has changed
    static void OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);

private:
    static TBUIRenderer* renderer;
    static Ptr<TBUIInputHandler> inputHandler;
    static Util::Array<TBUIView*> views;
    static CoreGraphics::VertexLayoutId vertexLayout;
    static CoreGraphics::ShaderId shader;
    static CoreGraphics::ShaderProgramId shaderProgram;
    static CoreGraphics::ResourceTableId resourceTable;
    static CoreGraphics::PipelineId pipeline;
    static IndexT textProjectionConstant;
    static IndexT textureConstant;

    static Util::Array<CoreGraphics::BufferId> usedVertexBuffers;
};

} // namespace TBUI
