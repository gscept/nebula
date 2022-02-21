//------------------------------------------------------------------------------
// framesubpassplugin.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameplugin.h"
#include "framesubpassplugin.h"

using namespace CoreGraphics;
namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FrameSubpassPlugin::FrameSubpassPlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FrameSubpassPlugin::~FrameSubpassPlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::Setup()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::Discard()
{
    FrameOp::Discard();

    this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled*
FrameSubpassPlugin::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();
    ret->func = this->func;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::Build(
    Memory::ArenaAllocator<BIG_CHUNK>& allocator
    , Util::Array<FrameOp::Compiled*>& compiledOps
    , Util::Array<CoreGraphics::EventId>& events
    , Util::Array<CoreGraphics::BarrierId>& barriers
    , Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers
    , Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures
)
{
    // if not enable, abort early
    if (!this->enabled)
        return;

    auto callback = Frame::GetCallback(this->name);
    if (callback != nullptr)
    {
        CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

        myCompiled->func = callback;
        this->compiled = myCompiled;

        // only setup sync if the function could be found
        if (myCompiled->func != nullptr)
            this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);
        compiledOps.Append(myCompiled);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSubpassPlugin::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    this->func(cmdBuf, frameIndex, bufferIndex);
}

} // namespace Frame2
