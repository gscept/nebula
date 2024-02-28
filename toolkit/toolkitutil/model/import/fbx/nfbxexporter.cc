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

#include <fbxsdk.h>

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
    wantedAxes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Z;
    wantedAxes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;

    ufbx_error error;
    ufbx_load_opts opts{.clean_skin_weights = true, .space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY, .target_axes = wantedAxes, .target_unit_meters = 1.0f};
    ufbx_scene* scene2 = ufbx_load_file_len(this->path.LocalPath().AsCharPtr(), this->path.LocalPath().Length(), &opts, &error);
    if (scene2 == nullptr)
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

    scene->Destroy(true);
    importer->Destroy(true);

    return true;
}

} // namespace ToolkitUtil