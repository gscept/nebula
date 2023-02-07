#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxNode
    
    Encapsulates an FBX node as a Nebula-friendly object
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "math/vec4.h"
#include "math/mat4.h"

#include <fbxsdk.h>


namespace ToolkitUtil
{

/// Convert FbxMatrix to Math::mat4
Math::mat4 FbxToMath(const fbxsdk::FbxMatrix& matrix);
/// Convert FbxMatrix to Math::mat4
Math::vec4 FbxToMath(const fbxsdk::FbxVector4& matrix);

class SceneNode;
class NFbxScene;
class NFbxMeshNode;
class ModelAttributes;
class AnimBuilder;
class AnimBuilderCurve;
class NFbxNode
{
public:

    /// Setup base node
    static void Setup(SceneNode* node, FbxNode* fbxNode);

    /// generates animation clip
    static void ExtractAnimation(SceneNode* node, FbxNode* fbxNode, FbxAnimStack* animStack);

protected:

    /// extract animation curves from node
    static void ExtractAnimationCurves(SceneNode* node, FbxNode* fbxNode, FbxAnimStack* stack);
}; 

} // namespace ToolkitUtil
//------------------------------------------------------------------------------