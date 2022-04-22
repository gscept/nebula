#pragma once
//------------------------------------------------------------------------------
/**
    TookitUtil::SurfaceBuilder

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"

namespace ToolkitUtil
{

class SurfaceBuilder
{
public:
    ///
    SurfaceBuilder();
    ///
    ~SurfaceBuilder();

    void SetDstDir(Util::String const& dir);
    void SetMaterial(Util::String const& name);
    void AddParam(Util::String const& name, Util::String const& value);

    void ExportBinary(Util::String const& dstFileName);

private:
    Util::String dstDir;
    Util::String material;
    Util::Array<Util::KeyValuePair<Util::String, Util::String>> params;
};

//------------------------------------------------------------------------------
/**
*/
inline void
SurfaceBuilder::SetDstDir(Util::String const& dir)
{
    this->dstDir = dir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
SurfaceBuilder::SetMaterial(Util::String const& name)
{
    this->material = name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
SurfaceBuilder::AddParam(Util::String const& name, Util::String const& value)
{
    this->params.Append({ name, value });
}

} // namespace ToolkitUtil
