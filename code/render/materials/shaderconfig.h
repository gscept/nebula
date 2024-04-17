#pragma once
//------------------------------------------------------------------------------
/**
    A material type declares the draw steps and associated shaders

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/hashtable.h"
#include "coregraphics/batchgroup.h"
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
    struct MaterialTemplateValue;
    struct MaterialTemplateTexture;
}
namespace Materials
{

struct ShaderConfigTexture
{
    Util::String name;
    CoreGraphics::TextureId defaultValue;
    CoreGraphics::TextureType type;
    bool system : 1;
};

struct ShaderConfigBatchTexture
{
    IndexT slot;
    const MaterialTemplates::MaterialTemplateTexture* def;
};

struct ShaderConfigConstant
{
    Util::String name;
    MaterialVariant def, min, max;
    bool system : 1;
};

struct ShaderConfigBatchConstant
{
    IndexT offset, slot, group;
    const MaterialTemplates::MaterialTemplateValue* def;

    // Returns true if valid
    const bool Valid() const
    {
        return offset != InvalidIndex && slot != InvalidIndex && group != InvalidIndex;
    }
};

} // namespace Materials
