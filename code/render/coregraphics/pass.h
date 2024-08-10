#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Pass
  
    A pass describe a set of textures used for rendering
    
    @copyright
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

struct ResourceTableId;
struct TextureView;
ID_24_8_TYPE(PassId);

enum class AttachmentFlagBits : uint16
{
    NoFlags = 0,
    Clear               = N_BIT(0),
    ClearStencil        = N_BIT(1),
    Load                = N_BIT(2),
    LoadStencil         = N_BIT(3),
    Store               = N_BIT(4),
    StoreStencil        = N_BIT(6),
    Discard             = N_BIT(7),
    DiscardStencil      = N_BIT(8),
};
__ImplementEnumBitOperators(AttachmentFlagBits);
__ImplementEnumComparisonOperators(AttachmentFlagBits);

static AttachmentFlagBits 
AttachmentFlagsFromString(const Util::String& string)
{
    Util::Array<Util::String> bits = string.Tokenize("|");

    AttachmentFlagBits ret = AttachmentFlagBits::NoFlags;
    for (IndexT i = 0; i < bits.Size(); i++)
    {
        if (bits[i] == "Load")                  ret |= AttachmentFlagBits::Load;
        else if (bits[i] == "LoadStencil")      ret |= AttachmentFlagBits::LoadStencil;
        else if (bits[i] == "Store")            ret |= AttachmentFlagBits::Store;
        else if (bits[i] == "StoreStencil")     ret |= AttachmentFlagBits::StoreStencil;
        else if (bits[i] == "Discard")          ret |= AttachmentFlagBits::Discard;
        else if (bits[i] == "DiscardStencil")   ret |= AttachmentFlagBits::DiscardStencil;
    }
    return ret;
};

struct Subpass
{
    Util::Array<IndexT> attachments;
    Util::Array<IndexT> resolves;
    Util::Array<IndexT> dependencies;
    Util::Array<IndexT> inputs;
    IndexT depthResolve;
    IndexT depth;
    SizeT numViewports;
    SizeT numScissors;

    Subpass() : depthResolve(InvalidIndex), depth(InvalidIndex), numViewports(0), numScissors(0) {};
};

struct PassCreateInfo
{
    Util::StringAtom name;

    Util::Array<CoreGraphics::TextureViewId> attachments;
    Util::Array<AttachmentFlagBits> attachmentFlags; 
    Util::Array<Math::vec4> attachmentClears;
    Util::Array<bool> attachmentDepthStencil;
    
    Util::Array<Subpass> subpasses;

    PassCreateInfo() {};
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
void DestroyPass(const PassId id);

/// called when window is resized
void PassWindowResizeCallback(const PassId id);

/// get number of color attachments for entire pass (attachment list)
const Util::Array<CoreGraphics::TextureViewId>& PassGetAttachments(const CoreGraphics::PassId id);

/// get number of color attachments for a subpass
const uint32_t PassGetNumSubpassAttachments(const CoreGraphics::PassId id, const IndexT subpass);
/// Get pass resource table
const CoreGraphics::ResourceTableId PassGetResourceTable(const CoreGraphics::PassId id);

/// get name
const Util::StringAtom PassGetName(const CoreGraphics::PassId id);

} // namespace CoreGraphics

