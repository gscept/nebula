#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::GltfFileImporter

    Exports an GLTF file into the binary nebula format

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "model/import/base/modelimporter.h"
#include "toolkit-common/base/exporttypes.h"
#include "gltfdata.h"
#include "node/ngltfscene.h"
#include "toolkitutil/texutil/textureconverter.h"

namespace ToolkitUtil
{
class GltfFileImporter : public ModelImporter
{
    __DeclareClass(GltfFileImporter);
public:

    /// constructor
    GltfFileImporter();
    /// destructor
    virtual ~GltfFileImporter();

    /// Parse the FBX scene data
    bool ParseScene(ToolkitUtil::ImportFlags importFlags, float scale) override;

    /// set texture converter
    void SetTextureConverter(TextureConverter* texConv);

private:

    Gltf::Document gltfScene;
    TextureConverter* texConverter;
};

//------------------------------------------------------------------------------
/**
*/
inline void
GltfFileImporter::SetTextureConverter(TextureConverter* texConv)
{
    this->texConverter = texConv;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------