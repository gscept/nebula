#pragma once
//------------------------------------------------------------------------------
/**
    TookitUtil::SurfaceBuilder

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "toolkit-common/logger.h"

namespace ToolkitUtil
{

class SurfaceBuilder
{
public:
    ///
    SurfaceBuilder();
    ///
    ~SurfaceBuilder();

    void SetLogger(ToolkitUtil::Logger* logger);
    void SetDstDir(Util::String const& dir);
    void SetMaterial(Util::String const& name);
    void AddParam(Util::String const& name, Util::String const& value);

    void ExportBinary(Util::String const& dstFileName);

private:
    Util::String dstDir;
    Util::String material;
    ToolkitUtil::Logger* logger;
    Util::Array<Util::KeyValuePair<Util::String, Util::String>> params;
};

//------------------------------------------------------------------------------
/**
*/
inline void 
SurfaceBuilder::SetLogger(ToolkitUtil::Logger* logger)
{
    this->logger = logger;
}

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
