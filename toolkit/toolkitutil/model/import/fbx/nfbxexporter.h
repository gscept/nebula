#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxExporter
    
    Exports an FBX file into the binary nebula format
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "model/import/base/modelexporter.h"
#include "node/nfbxscene.h"
#include "toolkit-common/base/exporttypes.h"
#include "model/modelutil/modelphysics.h"
#include <fbxsdk/core/fbxmanager.h>
#include <fbxsdk/fileio/fbxiosettings.h>
#include <fbxsdk/fileio/fbxprogress.h>

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

extern fbxsdk::FbxManager* sdkManager;
extern fbxsdk::FbxIOSettings* ioSettings;
class NFbxExporter : public ModelExporter
{
    __DeclareClass(NFbxExporter);
public:

    /// constructor
    NFbxExporter();
    /// destructor
    virtual ~NFbxExporter();

    /// Parse the FBX scene data
    bool ParseScene() override;

    /// set the progress callback
    void SetFbxProgressCallback(fbxsdk::FbxProgressCallback progressCallback);
private:

    fbxsdk::FbxProgressCallback progressFbxCallback;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
NFbxExporter::SetFbxProgressCallback(fbxsdk::FbxProgressCallback progressCallback)
{
    this->progressFbxCallback = progressCallback;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------