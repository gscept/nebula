#pragma once
//------------------------------------------------------------------------------
/**
    @class Dynui::ImguiRenderer
    
    Nebula renderer for  the IMGUI dynamic UI library.
    
    @copyright
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/resourcetable.h"
#include "input/inputevent.h"
#include "coregraphics/pipeline.h"
#include "graphics/graphicscontext.h"
#include "coregraphics/vertexlayout.h"
#include "imguiinputhandler.h"
#include "memory/arenaallocator.h"
#include "frame/framecode.h"

struct ImDrawData;
struct ImFont;
namespace Dynui
{

struct ImguiRendererParams
{
    IndexT projVar;
    IndexT fontVar;
};

struct ImguiTextureId
{
    CoreGraphics::TextureId nebulaHandle;
    uint8 layer = 0;
    uint8 mip = 0;
};

class ImguiContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    ImguiContext();
    /// destructor
    virtual ~ImguiContext();

    /// Create context
    static void Create();
    /// Discard context
    static void Discard();

    /// set the screen dimensions to use when rendering the UI (all vertices will be mapped to these values)
    static void SetRectSize(SizeT width, SizeT height);
    
    /// handle event
    static bool HandleInput(const Input::InputEvent& event);

    /// called if the window size has changed
    static void OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);
    /// called before frame
    static void NewFrame(const Graphics::FrameContext& ctx);
    /// called before frame
    static void Render(const Graphics::FrameContext& ctx);

    struct ImguiState
    {
        ImguiRendererParams params;
        CoreGraphics::ShaderId uiShader;
        CoreGraphics::ShaderProgramId prog;
        CoreGraphics::PipelineId pipeline;

        ImguiTextureId fontTexture;
        //CoreGraphics::TextureId fontTexture;

        Util::FixedArray<CoreGraphics::BufferId> vbos;
        Util::FixedArray<CoreGraphics::BufferId> ibos;
        CoreGraphics::VertexLayoutId vlo;

        IndexT textureConstant;
        IndexT textProjectionConstant;
        IndexT packedTextureInfo;
        CoreGraphics::ResourceTableId resourceTable;
        //Ptr<CoreGraphics::BufferLock> vboBufferLock;
        //Ptr<CoreGraphics::BufferLock> iboBufferLock;
        Util::FixedArray<byte*> vertexPtrs;
        Util::FixedArray<byte*> indexPtrs;

        ImFont* normalFont;
        ImFont* smallFont;
        ImFont* boldFont;
        ImFont* itFont;

        Memory::ArenaAllocator<sizeof(Frame::FrameCode)> frameOpAllocator;

        Ptr<ImguiInputHandler> inputHandler;
        bool dockOverViewport;
    };
    static ImguiState state;

private:
    static void ImguiDrawFunction(const CoreGraphics::CmdBufferId cmdBuf);
    static void RecoverImGuiContextErrors();
};

} // namespace Dynui
