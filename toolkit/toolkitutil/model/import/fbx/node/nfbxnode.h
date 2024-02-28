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
#include "timing/time.h"

#include "ufbx/ufbx.h"


namespace ToolkitUtil
{

/// Convert FbxMatrix to Math::mat4
Math::mat4 FbxToMath(const fbxsdk::FbxMatrix& matrix);
/// Convert FbxVector4 to Math::vec4
Math::vec4 FbxToMath(const fbxsdk::FbxVector4& vec);
/// Convert FbxVector2 to Math::vec2
Math::vec2 FbxToMath(const fbxsdk::FbxVector2& vec);

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
    static void Setup(SceneNode* node, SceneNode* parent, ufbx_node* fbxNode);

    /// Generates animation clip
    static void ExtractAnimation(SceneNode* node, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, ufbx_anim_stack* animStack);
    /// Returns true if node is animated
    static void PrepareAnimation(SceneNode* node, ufbx_anim_stack* animStack);

protected:

    /// extract animation curves from node
    static void ExtractAnimationCurves(SceneNode* node, ufbx_node* fbxNode, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, ufbx_anim_stack* animStack);
}; 

} // namespace ToolkitUtil
//------------------------------------------------------------------------------