//------------------------------------------------------------------------------
// rendertexturebase.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "rendertexturebase.h"
#include "coregraphics/displaydevice.h"
#include "resources/resourcemanager.h"

using namespace Resources;
using namespace CoreGraphics;
namespace Base
{

__ImplementClass(Base::RenderTextureBase, 'RTEB', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
RenderTextureBase::RenderTextureBase() :
	window(nullptr),
	texture(nullptr),
	format(PixelFormat::InvalidPixelFormat),
	type(Texture::InvalidType),
	usage(InvalidAttachment),
	width(0),
	height(0),
	depth(0),
	layers(1),
	widthScale(1),
	heightScale(1),
	depthScale(1),
	isInPass(false),
	msaaEnabled(false),
	relativeSize(false),
	dynamicSize(false),	
	windowTexture(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
RenderTextureBase::~RenderTextureBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::Setup()
{
	if (this->windowTexture)
	{
		this->window = DisplayDevice::Instance()->GetCurrentWindow();
		const DisplayMode& mode = this->window->GetDisplayMode();
		this->width = mode.GetWidth();
		this->height = mode.GetHeight();
		this->depth = 1;

		// setup type and usage
		this->type = Texture::Texture2D;
		this->usage = ColorAttachment;

		// just create a texture natively without managing it
		this->texture = CoreGraphics::Texture::Create();
		this->texture->SetPixelFormat(this->window->GetDisplayMode().GetPixelFormat());
		this->texture->SetWidth(this->width);
		this->texture->SetHeight(this->height);
	}
	else
	{
		n_assert(this->width > 0 && this->height > 0 && this->depth > 0);
		n_assert(this->type == Texture::Texture2D || this->type == Texture::TextureCube || this->type == Texture::Texture2DArray || this->type == Texture::TextureCubeArray);
		n_assert(this->usage != InvalidAttachment);
		Ptr<CoreGraphics::Window> wnd = DisplayDevice::Instance()->GetCurrentWindow();
		if (this->relativeSize)
		{
			const DisplayMode& mode = wnd->GetDisplayMode();
			this->width = SizeT(mode.GetWidth() * this->widthScale);
			this->height = SizeT(mode.GetHeight() * this->heightScale);
			this->depth = 1;
		}
		else if (this->dynamicSize)
		{
			// add scale factor here
			const DisplayMode& mode = wnd->GetDisplayMode();
			this->width = SizeT(mode.GetWidth() * this->widthScale);
			this->height = SizeT(mode.GetHeight() * this->heightScale);
			this->depth = 1;
		}

		// multiply layers by 6 if cube, calculate amount of textures by dividing by 6
		if (this->type == Texture::TextureCube || this->type == Texture::TextureCubeArray) this->layers *= 6;

		// setup texture resource
		if (this->resourceId.IsValid())
		{
			this->texture = ResourceManager::Instance()->CreateUnmanagedResource(this->resourceId, Texture::RTTI).downcast<Texture>();
		}
		else
		{
			// just create a texture natively without managing it
			this->texture = CoreGraphics::Texture::Create();
		}
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::Discard()
{
	n_assert(this->texture.isvalid());
	if (!this->windowTexture)
	{
		ResourceManager::Instance()->UnregisterUnmanagedResource(this->texture.upcast<Resource>());
		this->texture = 0;
	}	
	else
	{
		this->texture->SetState(Resource::Initial);
		this->texture = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::Resize()
{
	n_assert(this->width > 0 && this->height > 0 && this->depth > 0);
	n_assert(this->type == Texture::Texture2D || this->type == Texture::TextureCube);
	n_assert(this->usage != InvalidAttachment);
	Ptr<CoreGraphics::Window> wnd = DisplayDevice::Instance()->GetCurrentWindow();
	if (this->relativeSize)
	{
		const DisplayMode& mode = wnd->GetDisplayMode();
		this->width = SizeT(mode.GetWidth() * this->widthScale);
		this->height = SizeT(mode.GetHeight() * this->heightScale);
		this->depth = 1;
	}
	else if (this->dynamicSize)
	{
		// add scale factor here
		const DisplayMode& mode = wnd->GetDisplayMode();
		this->width = SizeT(mode.GetWidth() * this->widthScale);
		this->height = SizeT(mode.GetHeight() * this->heightScale);
		this->depth = 1;
	}

	this->texture->SetWidth(this->width);
	this->texture->SetHeight(this->height);
	this->texture->SetDepth(this->depth);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::GenerateMipChain()
{
	// empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::GenerateMipChain(IndexT from)
{
	n_assert(this->texture->numMipLevels > from);
	// empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::GenerateMipChain(IndexT from, IndexT to)
{
	n_assert(this->texture->numMipLevels > from && this->texture->numMipLevels > to);
	// empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::Blit(IndexT from, IndexT to, const Ptr<CoreGraphics::RenderTexture>& target)
{
	n_assert(this->texture->numMipLevels > from && this->texture->numMipLevels > to);
	// empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::SwapBuffers()
{
	n_assert(this->windowTexture);
	// implement in subclass
}


} // namespace Base