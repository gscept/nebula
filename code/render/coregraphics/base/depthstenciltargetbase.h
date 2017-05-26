#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::DepthStencilTargetBase
    
    A depth-stencil target is a low-level depth-stencil combined texture to which we perform draws.
    
    (C) 2013 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/texture.h"

namespace Base
{
class DepthStencilTargetBase : public Core::RefCounted
{
	__DeclareClass(DepthStencilTargetBase);
public:

	/// clear flags
	enum ClearFlag
	{
		ClearDepth = (1<<1),
		ClearStencil = (1<<2)
	};	

	/// constructor
	DepthStencilTargetBase();
	/// destructor
	virtual ~DepthStencilTargetBase();

	/// setup depth-stencil target
	virtual void Setup();
	/// discard depth-stencil target
	virtual void Discard();
	/// begin rendering to the render target
	virtual void BeginPass();
	/// end current render pass
	virtual void EndPass();

	/// set clear depth
	void SetClearDepth(float d);
	/// get clear depth
	float GetClearDepth() const;
	/// set clear stencil value
	void SetClearStencil(int s);
	/// get clear stencil value
	int GetClearStencil() const;
	/// set clear flags
	void SetClearFlags(uint clearFlags);
	/// get clear flags
	uint GetClearFlags() const;

	/// clear depth-stencil target
	virtual void Clear(uint flags);

	/// set resolve texture resource id
	void SetResolveTextureResourceId(const Resources::ResourceId& resId);
	/// get resolve texture resource id
	const Resources::ResourceId& GetResolveTextureResourceId() const;

	/// set render target width
	void SetWidth(SizeT w);
	/// get width of render target in pixels
	SizeT GetWidth() const;
	/// set render target height
	void SetHeight(SizeT h);
	/// get height of render target in pixels
	SizeT GetHeight() const;
	/// set render target relative width
	void SetRelativeWidth(float w);
	/// get render target relative width
	float GetRelativeWidth() const;
	/// set render target relative height
	void SetRelativeHeight(float h);
	/// get render target relative height
	float GetRelativeHeight() const;

	/// called after we change the display size
	void OnWindowResized(SizeT width, SizeT height);

	/// add a color buffer
	void SetDepthStencilBufferFormat(CoreGraphics::PixelFormat::Code colorFormat);
	/// get color buffer format at index
	CoreGraphics::PixelFormat::Code GetDepthStencilBufferFormat() const;

protected:
	uint clearFlags;
	float clearDepth;
	int clearStencil;
	bool isValid;
	bool inBeginPass;
	SizeT width;
	SizeT height;
	bool useRelativeSize;
	float relWidth;
	float relHeight;
	CoreGraphics::PixelFormat::Code format;
	Resources::ResourceId resolveTextureResId;
	Ptr<CoreGraphics::Texture> resolveDepthTexture;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
DepthStencilTargetBase::SetWidth( SizeT w )
{
	this->width = w;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT 
DepthStencilTargetBase::GetWidth() const
{
	return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DepthStencilTargetBase::SetHeight( SizeT h )
{
	this->height = h;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT 
DepthStencilTargetBase::GetHeight() const
{
	return this->height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DepthStencilTargetBase::SetClearDepth(float d)
{
	this->clearDepth = d;
}

//------------------------------------------------------------------------------
/**
*/
inline float
DepthStencilTargetBase::GetClearDepth() const
{
	return this->clearDepth;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DepthStencilTargetBase::SetClearStencil(int s)
{
	this->clearStencil = s;
}

//------------------------------------------------------------------------------
/**
*/
inline int
DepthStencilTargetBase::GetClearStencil() const
{
	return this->clearStencil;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DepthStencilTargetBase::SetClearFlags( uint clearFlags )
{
	this->clearFlags = clearFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
DepthStencilTargetBase::GetClearFlags() const
{
	return this->clearFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DepthStencilTargetBase::SetRelativeWidth( float w )
{
	this->relWidth = w;
	this->useRelativeSize = true;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
DepthStencilTargetBase::GetRelativeWidth() const
{
	return this->relWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DepthStencilTargetBase::SetRelativeHeight( float h )
{
	this->relHeight = h;
	this->useRelativeSize = true;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
DepthStencilTargetBase::GetRelativeHeight() const
{
	return this->relHeight;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DepthStencilTargetBase::SetResolveTextureResourceId(const Resources::ResourceId& resId)
{
	this->resolveTextureResId = resId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId&
DepthStencilTargetBase::GetResolveTextureResourceId() const
{
	return this->resolveTextureResId;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
DepthStencilTargetBase::SetDepthStencilBufferFormat( CoreGraphics::PixelFormat::Code colorFormat )
{
	n_assert(
		colorFormat == CoreGraphics::PixelFormat::D24X8 ||
		colorFormat == CoreGraphics::PixelFormat::D24S8 || 
		colorFormat == CoreGraphics::PixelFormat::D32S8 ||
		colorFormat == CoreGraphics::PixelFormat::D16S8
		);
	
	this->format = colorFormat;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::PixelFormat::Code 
DepthStencilTargetBase::GetDepthStencilBufferFormat() const
{
	return this->format;
}

} // namespace Base
//------------------------------------------------------------------------------