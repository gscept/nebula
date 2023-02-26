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
#include "foundation/timing/time.h"

#include <fbxsdk.h>


namespace ToolkitUtil
{

/// Convert FbxMatrix to Math::mat4
Math::mat4 FbxToMath(const fbxsdk::FbxMatrix& matrix);
/// Convert FbxVector4 to Math::vec4
Math::vec4 FbxToMath(const fbxsdk::FbxVector4& vec);
/// Convert FbxVector2 to Math::vec2
Math::vec2 FbxToMath(const fbxsdk::FbxVector2& vec);
/// Truncate double to float
float TruncDouble(const double d);

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
    static void Setup(SceneNode* node, SceneNode* parent, FbxNode* fbxNode);

    /// Generates animation clip
    static void ExtractAnimation(SceneNode* node, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, FbxAnimStack* animStack);
    /// Returns true if node is animated
    static void PrepareAnimation(SceneNode* node, FbxAnimStack* animStack);

protected:

    /// extract animation curves from node
    static void ExtractAnimationCurves(SceneNode* node, FbxNode* fbxNode, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, FbxAnimStack* stack);
}; 

} // namespace ToolkitUtil
//------------------------------------------------------------------------------