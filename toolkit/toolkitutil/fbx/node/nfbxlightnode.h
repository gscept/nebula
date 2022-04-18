#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxLightNode
    
    Represents an FBX light as a nebula light
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "fbx/node/nfbxnode.h"
namespace ToolkitUtil
{
class NFbxLightNode : public NFbxNode
{
    __DeclareClass(NFbxLightNode);
public:

    enum LightType
    {
        Spotlight,
        Pointlight,
        AreaLight,
        InvalidLight,

        NumLights
    };

    /// constructor
    NFbxLightNode();
    /// destructor
    virtual ~NFbxLightNode();

    /// setup the node
    void Setup(FbxNode* node, const Ptr<NFbxScene>& scene);

private:

    FbxLight*                   light;
    LightType                   lightType;
}; 
} // namespace ToolkitUtil
//------------------------------------------------------------------------------