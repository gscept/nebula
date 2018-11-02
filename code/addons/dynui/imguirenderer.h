#pragma once
//------------------------------------------------------------------------------
/**
	@class Dynui::ImguiRenderer
	
	Nebula renderer for  the IMGUI dynamic UI library.
	
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/texture.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "input/inputevent.h"
#include "graphics/graphicscontext.h"


struct ImDrawData;
namespace Dynui
{

extern void ImguiDrawFunction(ImDrawData* data);
struct ImguiRendererParams
{
	CoreGraphics::ConstantBinding projVar;
	CoreGraphics::ConstantBinding fontVar;
};

class ImguiContext : public Graphics::GraphicsContext
{
    _DeclareContext();
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

    /// called when rendering a frame batch
    static void OnRenderAsPlugin(const IndexT frameIndex, const Timing::Time frameTime, const Util::StringAtom& filter);

    /// called if the window size has changed
    static void OnWindowResized(IndexT windowId, SizeT width, SizeT height);
    /// called before frame
    static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);

private:

    static void ImguiDrawFunction(ImDrawData* data);
    struct ImguiState
    {
        ImguiRendererParams params;
        CoreGraphics::ShaderId uiShader;
        CoreGraphics::ShaderProgramId prog;
        CoreGraphics::TextureId fontTexture;
        CoreGraphics::VertexBufferId vbo;
        CoreGraphics::IndexBufferId ibo;

        CoreGraphics::ConstantBinding textureConstant;
        CoreGraphics::ConstantBinding textProjectionConstant;
        //Ptr<CoreGraphics::BufferLock> vboBufferLock;
        //Ptr<CoreGraphics::BufferLock> iboBufferLock;
        byte* vertexPtr;
        byte* indexPtr;
    };
    static ImguiState state;
};

} // namespace Dynui