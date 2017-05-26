//------------------------------------------------------------------------------
// barrierbase.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "barrierbase.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include <tuple>

namespace Base
{

__ImplementClass(Base::BarrierBase, 'BARB', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
BarrierBase::BarrierBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BarrierBase::~BarrierBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierBase::AddRenderTexture(const Ptr<CoreGraphics::RenderTexture>& tex, Access leftAccess, Access rightAccess)
{
	this->renderTextures.Append(tex);
	this->renderTexturesAccess.Append(std::make_tuple(leftAccess, rightAccess));
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierBase::AddReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex, Access leftAccess, Access rightAccess)
{
	this->readWriteTextures.Append(tex);
	this->readWriteTexturesAccess.Append(std::make_tuple(leftAccess, rightAccess));
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierBase::AddReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf, Access leftAccess, Access rightAccess)
{
	this->readWriteBuffers.Append(buf);
	this->readWriteBuffersAccess.Append(std::make_tuple(leftAccess, rightAccess));
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierBase::Setup()
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierBase::Discard()
{
	// empty, override in subclass
}

} // namespace Base