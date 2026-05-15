#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxExporter
    
    Exports an FBX file into the binary nebula format
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "model/import/base/modelimporter.h"
#include "node/nfbxscene.h"
#include "toolkit-common/base/exporttypes.h"
#include "model/modelutil/modelphysics.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class FbxFileImporter : public ModelImporter
{
    __DeclareClass(FbxFileImporter);
public:

    /// constructor
    FbxFileImporter();
    /// destructor
    virtual ~FbxFileImporter();

    /// Parse the FBX scene data
    bool ParseScene(ToolkitUtil::ImportFlags importFlags, float scale) override;
}; 


} // namespace ToolkitUtil
//------------------------------------------------------------------------------