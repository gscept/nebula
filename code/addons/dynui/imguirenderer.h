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

struct ImDrawData;
namespace Dynui
{

extern void ImguiDrawFunction(ImDrawData* data);
struct ImguiRendererParams
{
	CoreGraphics::ConstantBinding projVar;
	CoreGraphics::ConstantBinding fontVar;
};

class ImguiRenderer : public Core::RefCounted
{
	__DeclareClass(ImguiRenderer);
	__DeclareSingleton(ImguiRenderer);
public:
	/// constructor
	ImguiRenderer();
	/// destructor
	virtual ~ImguiRenderer();

	/// set the screen dimensions to use when rendering the UI (all vertices will be mapped to these values)
	void SetRectSize(SizeT width, SizeT height);
	
	/// setup the imgui renderer, call SetRectSize prior to this
	void Setup();
	/// discard imgui renderer
	void Discard();
	
	/// render frame
	void Render();
	/// handle event
	bool HandleInput(const Input::InputEvent& event);

private:

	friend void ImguiDrawFunction(ImDrawData* data);

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

} // namespace Dynui