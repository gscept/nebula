//------------------------------------------------------------------------------
// frameplugin.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
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
FramePlugin::CompiledImpl::Run(const IndexT frameIndex)
{
	this->func(frameIndex);
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

	ret->func = this->func;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::Build(
	Memory::ArenaAllocator<BIG_CHUNK>& allocator,
	Util::Array<FrameOp::Compiled*>& compiledOps,
	Util::Array<CoreGraphics::EventId>& events,
	Util::Array<CoreGraphics::BarrierId>& barriers,
	Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
	Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures)
{
	CompiledImpl* myCompiled = (CompiledImpl*)this->AllocCompiled(allocator);

	this->compiled = myCompiled;
	this->SetupSynchronization(allocator, events, barriers, rwBuffers, textures);
	compiledOps.Append(myCompiled);
}

Util::Dictionary<Util::StringAtom, std::function<void(IndexT)>> nameToFunction;

//------------------------------------------------------------------------------
/**
*/
const std::function<void(IndexT)>&
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
AddCallback(const Util::StringAtom name, std::function<void(IndexT)> func)
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