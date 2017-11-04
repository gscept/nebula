//------------------------------------------------------------------------------
// rendertexturebase.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "rendertexturebase.h"
#include "coregraphics/displaydevice.h"
#include "resources/resourcemanager.h"
#include "coregraphics/memorytexturepool.h"

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
	textureId(Ids::InvalidId64),
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
	// reserve resource
	this->textureId = Resources::ReserveResource(this->resourceName, "render_system", MemoryTexturePool::RTTI);
	Texture* tex = Resources::GetResource<Texture>(this->textureId);

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
		tex->SetPixelFormat(this->window->GetDisplayMode().GetPixelFormat());
		tex->SetWidth(this->width);
		tex->SetHeight(this->height);
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
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::Discard()
{
	n_assert(this->textureId != Ids::InvalidId64);
	Resources::DiscardResource(this->textureId);
	this->textureId = Ids::InvalidId64;
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
	Texture* tex = Resources::GetResource<Texture>(this->textureId);
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

	// set texture dimensions
	tex->SetWidth(this->width);
	tex->SetHeight(this->height);
	tex->SetDepth(this->depth);
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
	n_assert(this->mips > from);
	// empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::GenerateMipChain(IndexT from, IndexT to)
{
	n_assert(this->mips > from && this->mips > to);
	// empty, implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBase::Blit(IndexT from, IndexT to, const Ptr<CoreGraphics::RenderTexture>& target)
{
	n_assert(this->mips > from && this->mips > to);
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