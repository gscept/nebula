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

#include <fbxsdk.h>

using namespace Util;
using namespace IO;
using namespace ToolkitUtil;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::NFbxExporter, 'FBXE', Base::ExporterBase);


fbxsdk::FbxManager* sdkManager = nullptr;
fbxsdk::FbxIOSettings* ioSettings = nullptr;
Threading::CriticalSection cs;
//------------------------------------------------------------------------------
/**
*/
NFbxExporter::NFbxExporter() 
    : progressFbxCallback(0)
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
    cs.Enter();
    if (!sdkManager)
    {
        // Create the FBX SDK manager
        sdkManager = fbxsdk::FbxManager::Create();
        ioSettings = fbxsdk::FbxIOSettings::Create(sdkManager, "Import settings");
    }
    cs.Leave();

    auto scene = fbxsdk::FbxScene::Create(sdkManager, "Export scene");
    auto importer = fbxsdk::FbxImporter::Create(sdkManager, "Importer");

    bool importStatus = importer->Initialize(this->path.LocalPath().AsCharPtr(), -1, NULL);
    importer->SetProgressCallback(this->progressFbxCallback);
    if (importStatus)
    {
        importStatus = importer->Import(scene);
        if (!importStatus)
        {
            this->logger->Error("FBX - Failed to open\n");
            this->SetHasErrors(true);
            return false;
        }
    }
    else
    {
        this->logger->Error("FBX - Failed to initialize\n");
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