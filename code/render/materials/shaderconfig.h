#pragma once
//------------------------------------------------------------------------------
/**
    A material type declares the draw steps and associated shaders

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/hashtable.h"
#include "coregraphics/shader.h"
#include "memory/arenaallocator.h"
#include "util/variant.h"
#include "util/fixedarray.h"
#include "util/arraystack.h"
#include "material.h"
#include "materialvariant.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/graphicsdevice.h"

namespace MaterialTemplates
{
    struct MaterialTemplateTexture;
}
namespace Materials
{

struct ShaderConfigBatchTexture
{
    IndexT slot;
    const MaterialTemplates::MaterialTemplateTexture* def;
};

} // namespace Materials
