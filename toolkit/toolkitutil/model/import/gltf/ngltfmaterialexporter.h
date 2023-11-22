#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFMaterialExtractor

    Extracts and exports a GLTF file material into a Nebula surface binary file

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "gltfdata.h"
#include "surface/surfacebuilder.h"
#include "toolkit-common/logger.h"

namespace ToolkitUtil
{
class NglTFMaterialExtractor
{
public:
    /// constructor
    NglTFMaterialExtractor();
    /// destructor
    ~NglTFMaterialExtractor();

    void SetDocument(Gltf::Document const* document);
    void SetLogger(ToolkitUtil::Logger* logger);

    void SetCategoryName(Util::String const& categoryName);
    void SetExportSubDirectory(Util::String const& subDir);

    void ExportAll();

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
NglTFMaterialExtractor::SetDocument(Gltf::Document const* document)
{
    this->doc = document;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NglTFMaterialExtractor::SetLogger(ToolkitUtil::Logger* logger)
{
    this->logger = logger;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFMaterialExtractor::SetCategoryName(Util::String const& categoryName)
{
    this->catName = categoryName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFMaterialExtractor::SetExportSubDirectory(Util::String const& subDir)
{
    this->subDir = subDir;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------