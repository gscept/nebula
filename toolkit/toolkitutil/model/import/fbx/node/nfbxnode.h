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
struct ufbx_node;
struct ufbx_matrix;
struct ufbx_vec4;
struct ufbx_vec3;
struct ufbx_vec2;
struct ufbx_anim_stack;

namespace ToolkitUtil
{

/// Convert ufbx_matrix to Math::mat4
Math::mat4 FbxToMath(const ufbx_matrix& matrix);
/// Convert ufbx_vec4 to Math::vec4
Math::vec4 FbxToMath(const ufbx_vec4& vector);
/// Convert ufbx_vec3 to Math::vec3
Math::vec4 FbxToMath(const ufbx_vec3& vector);
/// Convert ufbx_vec2 to Math::vec2
Math::vec2 FbxToMath(const ufbx_vec2& vector);

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