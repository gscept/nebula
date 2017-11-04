//------------------------------------------------------------------------------
//  depthstenciltargetbase.cc
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resources/resourcemanager.h"
#include "coregraphics/depthstenciltarget.h"

namespace Base
{
__ImplementClass(Base::DepthStencilTargetBase, 'DSTB', Core::RefCounted);

using namespace Resources;
using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
DepthStencilTargetBase::DepthStencilTargetBase() :
	useRelativeSize(false),
	width(0),
	height(0),
	isValid(false),    
	inBeginPass(false),
	resolveDepthTexture(Ids::InvalidId64),
	resolveTextureResId(""),
	clearDepth(1),
	clearStencil(0),
	clearFlags(0),
	format(PixelFormat::D24S8)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
DepthStencilTargetBase::~DepthStencilTargetBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
DepthStencilTargetBase::Setup()
{
	n_assert(!this->isValid);
	this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
DepthStencilTargetBase::Discard()
{
	n_assert(this->isValid);
	n_assert(!this->inBeginPass);
	this->isValid = false;

	if (this->resolveDepthTexture != Ids::InvalidId64)
	{
		Resources::DiscardResource(this->resolveDepthTexture);
		this->resolveDepthTexture = Ids::InvalidId64;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
DepthStencilTargetBase::BeginPass()
{
	n_assert(this->isValid);
	n_assert(!this->inBeginPass);
	this->inBeginPass = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
DepthStencilTargetBase::EndPass()
{
	n_assert(this->isValid);
	n_assert(this->inBeginPass);
	this->inBeginPass = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
DepthStencilTargetBase::Clear(uint flags)
{
	n_error("DepthStencilTargetBase::Clear() not implemented!");
}

//------------------------------------------------------------------------------
/**
*/
void 
DepthStencilTargetBase::OnWindowResized(SizeT width, SizeT height)
{
	n_error("DepthStencilTargetBase::OnDisplayResized() not implemented!");
}
} // namespace Base