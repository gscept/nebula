#pragma once
//------------------------------------------------------------------------------
/**
	@class Base::ShaderReadWriteTextureBase

	Implements a way to create textures as non-resources, only used as data read-write dumps for shaders.

	Since no API automatically restricts access to images, being that shaders cannot determine when writes are done,
	the read-write texture must be unlocked prior to updating it, and locked prior to using it. This behavior guarantees
	the image data is never stomped.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resource.h"
#include "coregraphics/pixelformat.h"

namespace CoreGraphics
{
class Texture;
}

namespace Base
{
class ShaderReadWriteTextureBase : public Core::RefCounted
{
	__DeclareClass(ShaderReadWriteTextureBase)
public:
	/// constructor
	ShaderReadWriteTextureBase();
	/// destructor
	virtual ~ShaderReadWriteTextureBase();

	/// setup texture with fixed size
	virtual void Setup(const SizeT width, const SizeT height, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id);
	/// setup texture with relative size
	void SetupWithRelativeSize(const Math::scalar relWidth, const Math::scalar relHeight, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id);
	/// discard texture
	void Discard();
	/// set dimensions
	void SetDimensions(const float width, const float height, const float depth = 1);

	/// resize texture
	void Resize();
	/// clear texture
	void Clear(const Math::float4& clearColor);

	/// locks texture, blocking any GPU calls from working on it
	void Lock();
	/// unlocks texture, allowing the texture to be updated
	void Unlock();

	/// get underlying texture
	const Ptr<CoreGraphics::Texture>& GetTexture() const;
	/// get width
	const SizeT GetWidth() const;
	/// get height
	const SizeT GetHeight() const;

protected:
	bool useRelativeSize;
	uint lockSemaphore;
	Math::scalar relWidth, relHeight;
	Ptr<CoreGraphics::Texture> texture;
	SizeT width, height, depth;	
	CoreGraphics::PixelFormat pixelFormat;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Texture>&
ShaderReadWriteTextureBase::GetTexture() const
{
	return this->texture;
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT
ShaderReadWriteTextureBase::GetWidth() const
{
	return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT
ShaderReadWriteTextureBase::GetHeight() const
{
	return this->height;
}
} // namespace Base