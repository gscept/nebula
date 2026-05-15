#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::GltfFileMaterialExtractor

    Extracts and exports a GLTF file material into a Nebula surface binary file

    (C) 2020-2024 Individual contributors, see AUTHORS file
*/
#include "gltfdata.h"
#include "surface/surfacebuilder.h"
#include "toolkit-common/logger.h"

namespace ToolkitUtil
{
class GltfFileMaterialExtractor
{
public:
    /// constructor
    GltfFileMaterialExtractor();
    /// destructor
    ~GltfFileMaterialExtractor();

    void SetDocument(Gltf::Document const* document);
    void SetLogger(ToolkitUtil::Logger* logger);

    void SetCategoryName(Util::String const& categoryName);
    void SetExportSubDirectory(Util::String const& subDir);

    /// export all .sur files. returns the exported files paths
    Util::Array<IO::URI> ExportAll();

    void ExtractMaterial(SurfaceBuilder& builder, Gltf::Material const& material);
private:
    Gltf::Document const* doc;
    Util::String catName;
    Util::String subDir;

    // used when exporting
    Util::String texCatDir;
    Util::String textureDir;

    ToolkitUtil::Logger* logger;
};

//------------------------------------------------------------------------------
/**
*/
inline void
GltfFileMaterialExtractor::SetDocument(Gltf::Document const* document)
{
    this->doc = document;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
GltfFileMaterialExtractor::SetLogger(ToolkitUtil::Logger* logger)
{
    this->logger = logger;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GltfFileMaterialExtractor::SetCategoryName(Util::String const& categoryName)
{
    this->catName = categoryName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GltfFileMaterialExtractor::SetExportSubDirectory(Util::String const& subDir)
{
    this->subDir = subDir;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------