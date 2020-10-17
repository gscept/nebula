#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Pass
  
    A pass describe a set of textures used for rendering
    
    (C) 2015-2020 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "ids/idallocator.h"
#include "frame/framebatchtype.h"
#include "coregraphics/textureview.h"

namespace CoreGraphics
{

ID_24_8_TYPE(PassId);

enum class AttachmentFlagBits : uint8
{
	NoFlags = 0,
	Clear				= N_BIT(0),
	ClearStencil		= N_BIT(1),
	Load				= N_BIT(2),
	LoadStencil			= N_BIT(3),
	Store				= N_BIT(4),
	StoreStencil		= N_BIT(6)
};
__ImplementEnumBitOperators(AttachmentFlagBits);
__ImplementEnumComparisonOperators(AttachmentFlagBits);

struct Subpass
{
	Util::Array<IndexT> attachments;
	Util::Array<IndexT> dependencies;
	Util::Array<IndexT> inputs;
	SizeT numViewports;
	SizeT numScissors;
	bool bindDepth;
	bool resolve;

	Subpass() : bindDepth(false), resolve(false), numViewports(0), numScissors(0) {};
};

struct PassCreateInfo
{
	Util::StringAtom name;

	Util::Array<CoreGraphics::TextureViewId> colorAttachments;
	Util::Array<AttachmentFlagBits> colorAttachmentFlags; 
	Util::Array<Math::vec4> colorAttachmentClears;
	CoreGraphics::TextureViewId depthStencilAttachment;
	
	Util::Array<Subpass> subpasses;
	Frame::FrameBatchType::Code batchType;

	float clearDepth;
	uint clearStencil;
	AttachmentFlagBits depthStencilFlags;
};

enum class PassRecordMode : uint8
{
	Record,
	ExecuteRecorded,
	ExecuteInline
};

/// create pass
const PassId CreatePass(const PassCreateInfo& info);
/// discard pass
void DiscardPass(const PassId id);

/// begin using a pass
void PassBegin(const PassId id, PassRecordMode recordMode);
/// set currently bound pass to next subpass (asserts a valid pass is bound)
void PassNextSubpass(const PassId id);
/// end using a pass, this will set the pass id to be invalid
void PassEnd(const PassId id);

/// apply clip settings (viewport and scissor rect)
void PassApplyClipSettings(const PassId id);
/// apply pass resource table
void PassApplyResources(const PassId id);

/// called when window is resized
void PassWindowResizeCallback(const PassId id);

/// get number of color attachments for entire pass (attachment list)
const Util::Array<CoreGraphics::TextureViewId>& PassGetAttachments(const CoreGraphics::PassId id);
/// get depth stencil attachment
const CoreGraphics::TextureViewId PassGetDepthStencilAttachment(const CoreGraphics::PassId id);

/// get number of color attachments for a subpass
const uint32_t PassGetNumSubpassAttachments(const CoreGraphics::PassId id, const IndexT subpass);

/// get name
const Util::StringAtom PassGetName(const CoreGraphics::PassId id);

} // namespace CoreGraphics

