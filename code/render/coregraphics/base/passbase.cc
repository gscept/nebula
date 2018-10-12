//------------------------------------------------------------------------------
// framebufferbase.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "passbase.h"

namespace Base
{

__ImplementClass(Base::PassBase, 'FRBS', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
PassBase::PassBase() :
	depthStencilAttachment(NULL),
	depthStencilFlags(NoFlags),
	clearDepth(1),
	clearStencil(0),
	inBegin(false),
	inBatch(false),
	currentSubpass(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PassBase::~PassBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
PassBase::Setup()
{
	// override in implementation
}

//------------------------------------------------------------------------------
/**
*/
void
PassBase::Discard()
{
	// override in implementation
}

//------------------------------------------------------------------------------
/**
	Implement in subclass, but always call base class
*/
void
PassBase::Begin()
{
	n_assert(!this->inBegin);
	this->inBegin = true;
	this->currentSubpass = 0;

	// transition all color attachments to be within pass
	const Subpass& subpass = this->subpasses[this->currentSubpass];
	for (IndexT i = 0; i < subpass.attachments.Size(); i++)
	{
		this->colorAttachments[subpass.attachments[i]]->SetInPass(true);
	}
}

//------------------------------------------------------------------------------
/**
	Implement in subclass, but always call base class
*/
void
PassBase::NextSubpass()
{
	n_assert(this->inBegin);

	// transition previous attachments out of pass
	const Subpass& subpass = this->subpasses[this->currentSubpass];
	for (IndexT i = 0; i < subpass.attachments.Size(); i++)
	{
		this->colorAttachments[subpass.attachments[i]]->SetInPass(false);
	}

	// go to next pass
	this->currentSubpass++;

	// transition new subpass attachments into pass
	if (this->currentSubpass < (uint)this->subpasses.Size())
	{
		const Subpass& subpass = this->subpasses[this->currentSubpass];
		for (IndexT i = 0; i < subpass.attachments.Size(); i++)
		{
			this->colorAttachments[subpass.attachments[i]]->SetInPass(true);
		}
	}	
}

//------------------------------------------------------------------------------
/**
	Implement in subclass, but always call base class
*/
void
PassBase::End()
{
	n_assert(this->inBegin);
	this->inBegin = false;

	// transition all color attachments to be within pass
	const Subpass& subpass = this->subpasses[this->currentSubpass];
	for (IndexT i = 0; i < subpass.attachments.Size(); i++)
	{
		this->colorAttachments[subpass.attachments[i]]->SetInPass(false);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PassBase::BeginBatch(CoreGraphics::FrameBatchType::Code batchType)
{
	n_assert(this->inBegin);
	n_assert(!this->inBatch);
	this->inBatch = true;
	this->batchType = batchType;
}

//------------------------------------------------------------------------------
/**
*/
void
PassBase::EndBatch()
{
	n_assert(this->inBegin);
	n_assert(this->inBatch);
	this->inBatch = false;
	this->batchType = CoreGraphics::FrameBatchType::InvalidBatchType;
}

//------------------------------------------------------------------------------
/**
*/
void
PassBase::OnWindowResized()
{

}

} // namespace Base