//------------------------------------------------------------------------------
// frameplugin.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frameplugin.h"

namespace Frame
{

//------------------------------------------------------------------------------
/**
*/
FramePlugin::FramePlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FramePlugin::~FramePlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::CompiledImpl::Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex)
{
    this->func(cmdBuf, frameIndex, bufferIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::CompiledImpl::Discard()
{
    this->func = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
FrameOp::Compiled* 
FramePlugin::AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator)
{
    CompiledImpl* ret = allocator.Alloc<CompiledImpl>();

#if NEBULA_GRAPHICS_DEBUG
    ret->name = this->name;
#endif

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::Build(const BuildContext& ctx)
{
    // if not enable, abort early
    if (!this->enabled)
        return;
        
    auto callback = Frame::GetCallback(this->name);
    if (callback != nullptr)
    {
        CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(ctx.allocator);

        myCompiled->func = callback;
        this->compiled = myCompiled;

        // only setup sync if the function could be found
        if (myCompiled->func != nullptr)
            this->SetupSynchronization(ctx.allocator, ctx.events, ctx.barriers, ctx.buffers, ctx.textures);
        ctx.compiledOps.Append(myCompiled);
    }
}

Util::Dictionary<Util::StringAtom, std::function<void(const CoreGraphics::CmdBufferId, IndexT, IndexT)>> nameToFunction;

//------------------------------------------------------------------------------
/**
*/
const std::function<void(const CoreGraphics::CmdBufferId, IndexT, IndexT)>&
GetCallback(const Util::StringAtom& str)
{
    if (nameToFunction.Contains(str))
        return nameToFunction[str];
    else
    {
        n_printf("No function '%s' found\n", str.Value());
        return nameToFunction["null"];
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AddCallback(const Util::StringAtom name, std::function<void(const CoreGraphics::CmdBufferId, IndexT, IndexT)> func)
{
    nameToFunction.Add(name, func);
}

//------------------------------------------------------------------------------
/**
*/
void
InitPluginTable()
{
    nameToFunction.Add("null", nullptr);
}


} // namespace Frame2
