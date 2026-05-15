//------------------------------------------------------------------------------
//  fbxfileimporter.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "fbxfileimporter.h"
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
__ImplementClass(ToolkitUtil::FbxFileImporter, 'FBXI', Base::ImporterBase);

Threading::CriticalSection cs;
//------------------------------------------------------------------------------
/**
*/
FbxFileImporter::FbxFileImporter() 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FbxFileImporter::~FbxFileImporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
FbxFileImporter::ParseScene(ToolkitUtil::ImportFlags importFlags, float scale)
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
        .pivot_handling = UFBX_PIVOT_HANDLING_RETAIN, 
        .space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY,
        .target_axes = ufbx_axes_right_handed_y_up,
        .target_unit_meters = (ufbx_real)0.01f / scale
    };
    ufbx_scene* scene = ufbx_load_file_len(this->path.LocalPath().AsCharPtr(), this->path.LocalPath().Length(), &opts, &error);
    if (scene == nullptr)
    {
        this->logger->Error("FBX - Failed to open\n");
        this->SetHasErrors(true);
        return false;
    }

    auto fbxScene = new NFbxScene();
    fbxScene->SetName(this->file);
    fbxScene->SetCategory(this->folder);
    fbxScene->Setup(scene, importFlags, scale, this->logger);
    this->scene = fbxScene;

    ufbx_free_scene(scene);

    return true;
}

} // namespace ToolkitUtil