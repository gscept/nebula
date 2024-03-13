#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxNodeRegistry
    
    Parses an FBX scene and allocates Nebula-style nodes which can then be retrieved within the parsers
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "model/import/base/scene.h"
#include "model/meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "toolkit-common/logger.h"
#include "ufbx/ufbx.h"

namespace ToolkitUtil
{

class NFbxScene : public Scene
{
public:
    /// constructor
    NFbxScene();
    /// destructor
    virtual ~NFbxScene();

    /// sets up the scene
    void Setup(
        ufbx_scene* scene
        , const ToolkitUtil::ExportFlags& exportFlags
        , const Ptr<ModelAttributes>& attributes
        , float scale
        , ToolkitUtil::Logger* logger
    );

private:

    /// Parse FBX node hierarchy
    void ParseNodeHierarchy(
        ufbx_node* fbxNode
        , SceneNode* parent
        , Util::Dictionary<ufbx_node*, SceneNode*>& lookup
        , Util::Array<SceneNode>& nodes
    );
    /// converts FBX time mode to FPS
    float TimeModeToFPS(const ufbx_time_mode timeMode);

    ToolkitUtil::Logger* logger;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------