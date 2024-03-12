//------------------------------------------------------------------------------
//  fbxexporter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxexporter.h"
#include "io/ioserver.h"
#include "util/array.h"
#include "model/meshutil/meshbuildersaver.h"
#include "model/animutil/animbuildersaver.h"
#include "model/skeletonutil/skeletonbuildersaver.h"
#include "model/modelutil/modeldatabase.h"
#include "model/modelutil/modelattributes.h"
#include "ufbx/ufbx.h"

using namespace Util;
using namespace IO;
using namespace ToolkitUtil;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::NFbxExporter, 'FBXE', Base::ExporterBase);

Threading::CriticalSection cs;
//------------------------------------------------------------------------------
/**
*/
NFbxExporter::NFbxExporter() 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NFbxExporter::~NFbxExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
NFbxExporter::ParseScene()
{
    ufbx_coordinate_axes wantedAxes;
    wantedAxes.up = UFBX_COORDINATE_AXIS_POSITIVE_Y; 
    wantedAxes.front = UFBX_COORDINATE_AXIS_POSITIVE_Z;
    wantedAxes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;

    ufbx_error error;
    ufbx_load_opts opts 
    {
        .clean_skin_weights = true, 
        .strict = true, 
        .geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY,
        .pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT,
        .space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT,
        .target_axes = ufbx_axes_left_handed_y_up,
        .target_unit_meters = (ufbx_real)0.01f / this->sceneScale 
    };
    ufbx_scene* scene = ufbx_load_file_len(this->path.LocalPath().AsCharPtr(), this->path.LocalPath().Length(), &opts, &error);
    if (scene == nullptr)
    {
        this->logger->Error("FBX - Failed to open\n");
        this->SetHasErrors(true);
        return false;
    }

    // Lookup attributes used for take splitting
    Ptr<ModelAttributes> attributes = ModelDatabase::Instance()->LookupAttributes(this->category + "/" + this->file);

    auto fbxScene = new NFbxScene();
    fbxScene->SetName(this->file);
    fbxScene->SetCategory(this->category);
    fbxScene->Setup(scene, this->exportFlags, attributes, this->sceneScale, this->logger);
    this->scene = fbxScene;

    ufbx_free_scene(scene);

    return true;
}

} // namespace ToolkitUtil