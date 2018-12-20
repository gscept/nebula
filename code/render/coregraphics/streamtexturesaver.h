#pragma once
//------------------------------------------------------------------------------
/**
    @file CoreGraphics::StreamTextureSaver
    
    Allows to save texture data in a standard file format into a stream.

	Look in the renderer implementation for the implementation.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "resources/resourceid.h"
#include "io/uri.h"
#include "io/stream.h"
#include "imagefileformat.h"

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
	Save texture to path with wanted image format, and select mip map.
*/
bool
SaveTexture(const Resources::ResourceId& id, const IO::URI& path, IndexT mip, CoreGraphics::ImageFileFormat::Code code);

//------------------------------------------------------------------------------
/**
*/
bool
SaveTexture(const Resources::ResourceId& id, const Ptr<IO::Stream>& stream, IndexT mip, CoreGraphics::ImageFileFormat::Code code);

} // namespace CoreGraphics
//------------------------------------------------------------------------------

    