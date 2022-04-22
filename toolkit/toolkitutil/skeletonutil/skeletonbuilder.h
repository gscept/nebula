#pragma once
//------------------------------------------------------------------------------
/**
    Builds a skeleton to be saved as a resource

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"
#include "toolkitutil/n3util/n3modeldata.h"
namespace ToolkitUtil
{

struct SkeletonBuilder
{
    Util::Array<ToolkitUtil::Joint> joints;
};

} // namespace ToolkitUtil
