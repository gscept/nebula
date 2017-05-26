//------------------------------------------------------------------------------
// shaderreadwritetexturebase.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/shaderreadwritetexturebase.h"
#include "coregraphics/texture.h"
#include "resources/resourcemanager.h"
#include "../displaydevice.h"

namespace Base
{

__ImplementClass(Base::ShaderReadWriteTextureBase, 'SRTT', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ShaderReadWriteTextureBase::ShaderReadWriteTextureBase() :
	useRelativeSize(false),
	lockSemaphore(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderReadWriteTextureBase::~ShaderReadWriteTextureBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::Setup(const SizeT width, const SizeT height, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id)
{
	n_assert(id.IsValid());
	this->width = width;
	this->height = height;
	this->texture = CoreGraphics::Texture::Create();
	this->texture->SetResourceId(id);

	// register resource
	Resources::ResourceManager::Instance()->RegisterUnmanagedResource(this->texture.upcast<Resources::Resource>());
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::SetupWithRelativeSize(const Math::scalar relWidth, const Math::scalar relHeight, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id)
{
	n_assert(relWidth > 0);
	n_assert(relHeight > 0);

	// set relative size and 
	this->relWidth = relWidth;
	this->relHeight = relHeight;
	this->useRelativeSize = true;

	// setup texture with relative size
	CoreGraphics::DisplayMode mode = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow()->GetDisplayMode();
	SizeT width = SizeT(mode.GetWidth() * this->relWidth);
	SizeT height = SizeT(mode.GetHeight() * this->relHeight);
	this->Setup(width, height, format, id);
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::Discard()
{
	n_assert(this->texture.isvalid());
	Resources::ResourceManager::Instance()->UnregisterUnmanagedResource(this->texture.upcast<Resources::Resource>());
	this->texture = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::Resize()
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::Clear(const Math::float4& clearColor)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::Lock()
{
	this->lockSemaphore++;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::Unlock()
{
	this->lockSemaphore--;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteTextureBase::SetDimensions(const float width, const float height, const float depth /*= 1*/)
{
	this->width = (SizeT)Math::n_max(width, 1.0f);
	this->height = (SizeT)Math::n_max(height, 1.0f);
	this->depth = (SizeT)Math::n_max(depth, 1.0f);
	this->relWidth = width;
	this->relHeight = height;
}

} // namespace Base