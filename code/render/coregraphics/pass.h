#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Pass
  
    A pass describe a set of textures used for rendering
    
    (C) 2015 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "ids/idallocator.h"
#include "rendertexture.h"
#include "frame/framebatchtype.h"

namespace CoreGraphics
{

ID_24_8_TYPE(PassId);

enum AttachmentFlagBits
{
	NoFlags = 0,
	Clear = 1 << 0,
	ClearStencil = 1 << 1,
	Load = 1 << 2,
	LoadStencil = 1 << 3,
	Store = 1 << 4,
	StoreStencil = 1 << 5
};

struct Subpass
{
	Util::Array<IndexT> attachments;
	Util::Array<IndexT> dependencies;
	Util::Array<IndexT> inputs;
	bool bindDepth;
	bool resolve;

	Subpass() : bindDepth(false), resolve(false) {};
};

struct PassCreateInfo
{
	Util::StringAtom name;

	Util::Array<CoreGraphics::RenderTextureId> colorAttachments;
	Util::Array<AttachmentFlagBits> colorAttachmentFlags; 
	Util::Array<Math::float4> colorAttachmentClears;
	CoreGraphics::RenderTextureId depthStencilAttachment;	
	
	Util::Array<Subpass> subpasses;
	Frame::FrameBatchType::Code batchType;

	float clearDepth;
	uint clearStencil;
	AttachmentFlagBits depthStencilFlags;
};

/// create pass
const PassId CreatePass(const PassCreateInfo& info);
/// discard pass
void DiscardPass(const PassId& id);

/// begin using a pass
void PassBegin(const PassId& id);
/// begin batch within pass
void PassBeginBatch(const PassId& id, Frame::FrameBatchType::Code batch);
/// set currently bound pass to next subpass (asserts a valid pass is bound)
void PassNextSubpass(const PassId& id);
/// end batch within pass
void PassEndBatch(const PassId& id);
/// end using a pass, this will set the pass id to be invalid
void PassEnd(const PassId& id);

/// called when window is resized
void PassWindowResizeCallback(const PassId& id);

/// get number of color attachments for pass and subpass
const Util::Array<CoreGraphics::RenderTextureId>& PassGetAttachments(const CoreGraphics::PassId& id, const IndexT subpass);

} // namespace CoreGraphics

