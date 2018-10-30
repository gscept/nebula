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

namespace Dynui
{
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

	/// get buffer lock for vertex buffer
	//const Ptr<CoreGraphics::BufferLock>& GetVertexBufferLock() const;
	/// get buffer lock for index buffer
	//const Ptr<CoreGraphics::BufferLock>& GetIndexBufferLock() const;
	/// get vertex buffer pointer
	byte* GetVertexPtr() const;
	/// get vertex buffer pointer
	byte* GetIndexPtr() const;
	/// get vertex buffer
    CoreGraphics::VertexBufferId GetVertexBuffer() const;
	/// get index buffer
    CoreGraphics::IndexBufferId GetIndexBuffer() const;
	/// get shader
    CoreGraphics::ShaderId GetShader() const;
	/// get font texture
    CoreGraphics::TextureId GetFontTexture() const;
	/// get shader params
	const ImguiRendererParams& GetParams() const;

private:

	ImguiRendererParams params;
    CoreGraphics::ShaderId uiShader;
	CoreGraphics::TextureId fontTexture;
	CoreGraphics::VertexBufferId vbo;
	CoreGraphics::IndexBufferId ibo;
	//Ptr<CoreGraphics::BufferLock> vboBufferLock;
	//Ptr<CoreGraphics::BufferLock> iboBufferLock;
	byte* vertexPtr;
	byte* indexPtr;
};

#if 0
//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::BufferLock>&
ImguiRenderer::GetVertexBufferLock() const
{
	return this->vboBufferLock;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::BufferLock>&
ImguiRenderer::GetIndexBufferLock() const
{
	return this->iboBufferLock;
}
#endif
//------------------------------------------------------------------------------
/**
*/
inline byte*
ImguiRenderer::GetVertexPtr() const
{
	return this->vertexPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline byte*
ImguiRenderer::GetIndexPtr() const
{
	return this->indexPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::VertexBufferId
ImguiRenderer::GetVertexBuffer() const
{
	return this->vbo;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::IndexBufferId
ImguiRenderer::GetIndexBuffer() const
{
	return this->ibo;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::ShaderId
ImguiRenderer::GetShader() const
{
	return this->uiShader;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::TextureId
ImguiRenderer::GetFontTexture() const
{
	return this->fontTexture;
}

//------------------------------------------------------------------------------
/**
*/
inline const ImguiRendererParams&
ImguiRenderer::GetParams() const
{
	return this->params;
}

} // namespace Dynui