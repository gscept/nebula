#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::PostEffectParser

    Loads and saves posteffect paramsets into xml files

    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "posteffect/posteffectentity.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class PostEffectParser
{
public:
    /// read from xml file
    static bool Load(const Util::String & path, PostEffect::PostEffectEntity::ParamSet & parms);

    /// save to xml file
    static void Save(const Util::String & path, const PostEffect::PostEffectEntity::ParamSet & parms);
};
}

