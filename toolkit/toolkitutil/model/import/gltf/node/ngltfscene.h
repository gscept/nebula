#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::NglTFScene

    Parses a glTF scene and allocates Nebula-style nodes which can then be retrieved within the parsers

    (C) 2020 Individual contributors, see AUTHORS file
*/

#include "model/import/base/scene.h"
#include "ngltfmesh.h"
#include "ngltfnode.h"
#include "model/meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "model/import/gltf/gltfdata.h"
#include "meshprimitive.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class NglTFScene : public Scene
{
public:
    /// constructor
    NglTFScene();
    /// destructor
    virtual ~NglTFScene();

    /// sets up the scene
    void Setup(Gltf::Document* scene, const ToolkitUtil::ExportFlags& exportFlags, float scale);
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------