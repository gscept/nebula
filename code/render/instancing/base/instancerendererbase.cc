//------------------------------------------------------------------------------
//  instancerendererbase.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "instancerendererbase.h"
#include "coregraphics/transformdevice.h"

using namespace CoreGraphics;
using namespace Math;
namespace Base
{
__ImplementClass(Base::InstanceRendererBase, 'INRB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
InstanceRendererBase::InstanceRendererBase() :
	isOpen(false),
	inBeginUpdate(false),
	shader(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
InstanceRendererBase::~InstanceRendererBase()
{
	this->shader = 0;
	this->modelTransforms.Clear();
	this->modelViewTransforms.Clear();
	this->modelViewProjectionTransforms.Clear();
    this->objectIds.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
InstanceRendererBase::Setup()
{
	n_assert(!this->isOpen);
	this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
InstanceRendererBase::Close()
{
	n_assert(this->isOpen);
	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
InstanceRendererBase::BeginUpdate(SizeT amount)
{
	n_assert(!this->inBeginUpdate);
	this->modelTransforms.Clear();
	this->modelViewTransforms.Clear();
	this->modelViewProjectionTransforms.Clear();
    this->objectIds.Clear();
	this->modelTransforms.Reserve(amount);
	this->modelViewTransforms.Reserve(amount);
	this->modelViewProjectionTransforms.Reserve(amount);
    this->objectIds.Reserve(amount);
	this->inBeginUpdate = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
InstanceRendererBase::AddTransform( const matrix44& matrix )
{
	n_assert(this->inBeginUpdate);
	this->modelTransforms.Append(matrix);
}

//------------------------------------------------------------------------------
/**
*/
void 
InstanceRendererBase::AddId( const int id )
{
    n_assert(this->inBeginUpdate);
    this->objectIds.Append(id);
}

//------------------------------------------------------------------------------
/**
	Assumes all transforms has been set. 
	Calculate remaining transforms.
*/
void 
InstanceRendererBase::EndUpdate()
{
	n_assert(this->inBeginUpdate);
	this->inBeginUpdate = false;

	// get view and projection transforms
	const Ptr<TransformDevice>& transDev = TransformDevice::Instance();
	const matrix44& view = transDev->GetViewTransform();
	const matrix44& viewProj = transDev->GetViewProjTransform();

	// calculate remainder of transforms
	IndexT i;
	for (i = 0; i < this->modelTransforms.Size(); i++)
	{
		// get base transform
		const matrix44& trans = this->modelTransforms[i];

		// add transforms
		this->modelViewTransforms.Append(matrix44::multiply(trans, view));
		this->modelViewProjectionTransforms.Append(matrix44::multiply(trans, viewProj));
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
InstanceRendererBase::Render(const SizeT multiplier)
{
	// override in subclass
	n_error("InstanceRendererBase::Render() called!\n");
}


} // namespace Instancing