#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFExporter

    Exports an GLTF file into the binary nebula format

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "model/import/base/modelexporter.h"
#include "toolkit-common/base/exporttypes.h"
#include "gltf/gltfdata.h"
#include "node/ngltfscene.h"
#include "toolkitutil/texutil/textureconverter.h"

namespace ToolkitUtil
{
class NglTFExporter : public ModelExporter
{
    __DeclareClass(NglTFExporter);
public:

    /// constructor
    NglTFExporter();
    /// destructor
    virtual ~NglTFExporter();

    /// Parse the FBX scene data
    bool ParseScene() override;

    /// set texture converter
    void SetTextureConverter(TextureConverter* texConv);

private:

    Gltf::Document gltfScene;
    Util::Array<Util::String> exportedMeshes;
    TextureConverter* texConverter;
};

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFExporter::SetTextureConverter(TextureConverter* texConv)
{
    this->texConverter = texConv;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------