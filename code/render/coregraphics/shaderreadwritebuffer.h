#pragma once
//------------------------------------------------------------------------------
/**
	A shader read-write buffer can be both read and written from within a shader.

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
namespace CoreGraphics
{
ID_24_8_TYPE(ShaderRWBufferId);

struct ShaderRWBufferCreateInfo
{
	SizeT size;
	SizeT numBackingBuffers;
	bool screenRelative : 1; // when set, size is bytes per pixel
};

/// create shader RW buffer
const ShaderRWBufferId CreateShaderRWBuffer(const ShaderRWBufferCreateInfo& info);
/// destroy shader RW buffer
void DestroyShaderRWBuffer(const ShaderRWBufferId id);

} // CoreGraphics
