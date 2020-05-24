#pragma once
//------------------------------------------------------------------------------
/**
	Utility functions to convert from gliml/dds types to nebula

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "coregraphics/pixelformat.h"
#define GLIML_ASSERT(x) n_assert(x)
#include "gliml.h"
namespace CoreGraphics
{

class Gliml
{
public:
	static CoreGraphics::PixelFormat::Code ToPixelFormat(gliml::context const& ctx);
};

}