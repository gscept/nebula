//------------------------------------------------------------------------------
// frameplugin.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameplugin.h"

namespace Frame
{
Util::Dictionary<Util::StringAtom, std::function<void(IndexT)>> FramePlugin::nameToFunction;

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
FramePlugin::Setup()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::Discard()
{
	// override in subclass for any special discard behavior
}

//------------------------------------------------------------------------------
/**
*/
const std::function<void(IndexT)>&
FramePlugin::GetCallback(const Util::StringAtom& str)
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
FramePlugin::AddCallback(const Util::StringAtom name, std::function<void(IndexT)> func)
{
	nameToFunction.Add(name, func);
}

//------------------------------------------------------------------------------
/**
*/
void 
FramePlugin::UpdateResources(const IndexT frameIndex)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void 
FramePlugin::UpdateViewDependentResources(const Ptr<Graphics::View>& view, const IndexT frameIndex)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::Resize()
{
	IndexT i;
	for (i = 0; i < this->textures.Size(); i++)
		TextureWindowResized(this->textures.ValueAtIndex(i));
}

//------------------------------------------------------------------------------
/**
*/
void
FramePlugin::InitPluginTable()
{
	FramePlugin::nameToFunction.Add("null", nullptr);
}

} // namespace Frame
