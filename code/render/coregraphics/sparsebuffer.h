#pragma once
//------------------------------------------------------------------------------
/**
    Virtually mapped buffer

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/config.h"
namespace CoreGraphics
{

/// sparse buffer type
RESOURCE_ID_TYPE(SparseBufferId);

struct SparseBufferCreateInfo
{

};

/// create sparse buffer
SparseBufferId CreateSparseBuffer(const SparseBufferCreateInfo& info);
/// destroy sparse buffers
void DestroySparseBuffer(const SparseBufferId id);

} // namespace CoreGraphics
