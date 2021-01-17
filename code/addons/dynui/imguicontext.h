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
#include "graphics/graphicscontext.h"
#include "coregraphics/vertexlayout.h"
#include "imguiinputhandler.h"

struct ImDrawData;
namespace Dynui
{

struct ImguiRendererParams
{
    IndexT projVar;
    IndexT fontVar;
};

struct ImguiTextureId

{
    uint64 nebulaHandle;
    uint8 layer;
    uint8 mip;
};

class ImguiContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    ImguiContext();
    /// destructor
    virtual ~ImguiContext();

    static void Create();
    static void Discard();

    /// set the screen dimensions to use when rendering the UI (all vertices will be mapped to these values)
    static void SetRectSize(SizeT width, SizeT height);
    
    /// handle event
    static bool HandleInput(const Input::InputEvent& event);

    /// called if the window size has changed
    static void OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);
    /// called before frame
    static void OnBeforeFrame(const Graphics::FrameContext& ctx);

private:
    struct ImguiState
    {
        ImguiRendererParams params;
        CoreGraphics::ShaderId uiShader;
        CoreGraphics::ShaderProgramId prog;

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

        Ptr<ImguiInputHandler> inputHandler;
    };
    static ImguiState state;
    static void ImguiDrawFunction();
    static void RecoverImGuiContextErrors();
};

} // namespace Dynui
