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

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

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
}; 


} // namespace ToolkitUtil
//------------------------------------------------------------------------------