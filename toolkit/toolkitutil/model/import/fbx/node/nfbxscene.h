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

#include <fbxsdk/core/base/fbxtime.h>

namespace fbxsdk
{
    class FbxScene;
}

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
        fbxsdk::FbxScene* scene
        , const ToolkitUtil::ExportFlags& exportFlags
        , const Ptr<ModelAttributes>& attributes
        , float scale
    );

    /// Extract skeleton roots
    void ExtractSkeletons();
    /// Extract animations
    void ExtractAnimations(const Ptr<ToolkitUtil::ModelAttributes>& attributes);

private:

    /// converts FBX time mode to FPS
    float TimeModeToFPS(const fbxsdk::FbxTime::EMode& timeMode);
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------