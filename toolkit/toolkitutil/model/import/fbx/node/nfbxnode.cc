//------------------------------------------------------------------------------
//  fbxnode.h.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxnode.h"
#include "nfbxscene.h"
#include "model/modelutil/clip.h"
#include "model/modelutil/clipevent.h"
#include "model/modelutil/take.h"
#include "model/modelutil/modelattributes.h"
#include "util/set.h"
#include "model/import/base/uniquestring.h"
#include "ufbx/ufbx.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace CoreAnimation;
using namespace Util;
using namespace ToolkitUtil;

//------------------------------------------------------------------------------
/**
*/
Math::mat4
FbxToMath(const ufbx_matrix& matrix)
{
    Math::mat4 ret;
    ret.set(
        matrix.cols[0].v[0], matrix.cols[0].v[1], matrix.cols[0].v[2], 0.0,
        matrix.cols[1].v[0], matrix.cols[1].v[1], matrix.cols[1].v[2], 0.0,
        matrix.cols[2].v[0], matrix.cols[2].v[1], matrix.cols[2].v[2], 0.0,
        matrix.cols[3].v[0], matrix.cols[3].v[1], matrix.cols[3].v[2], 1.0);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
FbxToMath(const ufbx_vec3& vector)
{
    Math::vec3 ret;
    ret.set(
        vector.x
        , vector.y
        , vector.z
    );
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
FbxToMath(const ufbx_vec4& vector)
{
    Math::vec4 ret;
    ret.set(
        vector.x
        , vector.y
        , vector.z
        , vector.w
    );
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::quat
FbxToMath(const ufbx_quat& quat)
{
    Math::quat ret;
    ret.set(
        quat.x
        , quat.y
        , quat.z
        , quat.w
    );
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2
FbxToMath(const ufbx_vec2& vector)
{
    Math::vec2 ret;
    ret.set(
        vector.x
        , vector.y
    );
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::Setup(SceneNode* node, SceneNode* parent, ufbx_node* fbxNode)
{
    node->base.name = UniqueString::New(fbxNode->name.data);
    if (node->base.name == "physics")
    {
        node->base.isPhysics = true;
    }

    ufbx_prop* translationProp = ufbx_find_prop(&fbxNode->props, UFBX_Lcl_Translation); 
    if (translationProp != nullptr)
        node->base.translation = FbxToMath(translationProp->value_vec3);
    else
        node->base.translation = xyz(FbxToMath(fbxNode->local_transform.translation));
    
    ufbx_prop* rotationProp = ufbx_find_prop(&fbxNode->props, UFBX_Lcl_Rotation);
    if (rotationProp != nullptr)
        node->base.rotation = FbxToMath(ufbx_euler_to_quat(rotationProp->value_vec3, fbxNode->rotation_order));
    else
        node->base.rotation = FbxToMath(fbxNode->local_transform.rotation);

    ufbx_prop* scalingProp = ufbx_find_prop(&fbxNode->props, UFBX_Lcl_Scaling);
    if (scalingProp != nullptr)
        node->base.scale = FbxToMath(scalingProp->value_vec3);
    else
        node->base.scale = xyz(FbxToMath(fbxNode->local_transform.scale));

    node->fbx.node = fbxNode;
    node->base.parent = parent;
    if (parent)
        parent->base.children.Append(node);
}

//------------------------------------------------------------------------------
/**
    Generates animation clips based on node type. Mesh and transform nodes typically has a single set of curves, joint nodes typically has a tree.
*/
void
NFbxNode::ExtractAnimation(SceneNode* node, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, ufbx_anim_stack* animStack)
{
    // Run recursive function to collect animation curves this node hierarchy
    std::function<void(SceneNode*)> recursiveExtract = [&](SceneNode* node)
    {
        ExtractAnimationCurves(node, node->fbx.node, keys, keyTimes, animStack);
        for (const auto& child : node->base.children)
            recursiveExtract(child);
    };
    recursiveExtract(node);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
CountKeys(ufbx_anim_curve* fbxCurveX, ufbx_anim_curve* fbxCurveY, ufbx_anim_curve* fbxCurveZ, Timing::Tick sampleRate, Util::Set<double>& times)
{
    // If we have a sample rate, extract keys based on sample frequency
    if (sampleRate != 0)
    {
        double minTime = DBL_MAX, maxTime = -DBL_MAX;
        if (fbxCurveX != nullptr)
        {
            minTime = min(fbxCurveX->keyframes.begin()->time, minTime);
            maxTime = max(fbxCurveX->keyframes.end()->time, maxTime);
        }
        if (fbxCurveY != nullptr)
        {
            minTime = min(fbxCurveY->keyframes.begin()->time, minTime);
            maxTime = max(fbxCurveY->keyframes.end()->time, maxTime);
        }
        if (fbxCurveZ != nullptr)
        {
            minTime = min(fbxCurveZ->keyframes.begin()->time, minTime);
            maxTime = max(fbxCurveZ->keyframes.end()->time, maxTime);
        }
        double keyEvaluationPoint = minTime;
        while (keyEvaluationPoint < maxTime)
        {
            times.Add(keyEvaluationPoint);
            keyEvaluationPoint += sampleRate / 1000.0;
        }
    }
    else
    {
        if (fbxCurveX != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveX->keyframes.count; keyIndex++)
            {
                times.Add(fbxCurveX->keyframes[keyIndex].time);
            }
        }
        if (fbxCurveY != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveY->keyframes.count; keyIndex++)
            {
                times.Add(fbxCurveY->keyframes[keyIndex].time);
            }
        }
        if (fbxCurveZ != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveZ->keyframes.count; keyIndex++)
            {
                times.Add(fbxCurveZ->keyframes[keyIndex].time);
            }
        }
    }
    return times.Size();
};

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::PrepareAnimation(SceneNode* node, ufbx_anim_stack* animStack)
{
    // we only need the base (which contains the sum of all layers)
    ufbx_node* fbxNode = node->fbx.node;
    ufbx_anim_layer* animLayer = animStack->layers[0];

    //ufbx_prop* prop = ufbx_get_prop_element(&fbxNode->element, &fbxNode->props, UFBX_Lcl_Translation);

    ufbx_prop* rotationPivot = ufbx_find_prop(&node->fbx.node->props, UFBX_RotationPivot);
    ufbx_prop* scalePivot = ufbx_find_prop(&node->fbx.node->props, UFBX_ScalingPivot);

    ufbx_anim_prop* translationProperty = ufbx_find_anim_prop(animLayer, &fbxNode->element, UFBX_Lcl_Translation);
    ufbx_anim_prop* rotationProperty = ufbx_find_anim_prop(animLayer, &fbxNode->element, UFBX_Lcl_Rotation);
    ufbx_anim_prop* scalingProperty = ufbx_find_anim_prop(animLayer, &fbxNode->element, UFBX_Lcl_Scaling);

    ufbx_anim_curve* translationCurveX = nullptr;
    ufbx_anim_curve* translationCurveY = nullptr;
    ufbx_anim_curve* translationCurveZ = nullptr;

    ufbx_anim_curve* rotationCurveX = nullptr;
    ufbx_anim_curve* rotationCurveY = nullptr;
    ufbx_anim_curve* rotationCurveZ = nullptr;

    ufbx_anim_curve* scaleCurveX = nullptr;
    ufbx_anim_curve* scaleCurveY = nullptr;
    ufbx_anim_curve* scaleCurveZ = nullptr;

    if (translationProperty)
    {
        translationCurveX = translationProperty->anim_value->curves[0];
        translationCurveY = translationProperty->anim_value->curves[1];
        translationCurveZ = translationProperty->anim_value->curves[2];
    }

    if (rotationProperty)
    {
        rotationCurveX = rotationProperty->anim_value->curves[0];
        rotationCurveY = rotationProperty->anim_value->curves[1];
        rotationCurveZ = rotationProperty->anim_value->curves[2];
    }

    if (scalingProperty)
    {
        scaleCurveX = scalingProperty->anim_value->curves[0];
        scaleCurveY = scalingProperty->anim_value->curves[1];
        scaleCurveZ = scalingProperty->anim_value->curves[2];
    } 

    AnimBuilderCurve& translationCurve = node->anim.translationCurve;
    AnimBuilderCurve& rotationCurve = node->anim.rotationCurve;
    AnimBuilderCurve& scaleCurve = node->anim.scaleCurve;

    translationCurve.curveType = CurveType::Translation;
    rotationCurve.curveType = CurveType::Rotation;
    scaleCurve.curveType = CurveType::Scale;

    ufbx_anim_curve* translations[] = { translationCurveX, translationCurveY, translationCurveZ };
    ufbx_anim_curve* rotations[] = { rotationCurveX, rotationCurveY, rotationCurveZ };
    ufbx_anim_curve* scalings[] = { scaleCurveX, scaleCurveY, scaleCurveZ };

    translationCurve.numKeys = CountKeys(translationCurveX, translationCurveY, translationCurveZ, 0, node->fbx.translationKeyTimes);
    rotationCurve.numKeys = CountKeys(rotationCurveX, rotationCurveY, rotationCurveZ, 0, node->fbx.rotationKeyTimes);
    scaleCurve.numKeys = CountKeys(scaleCurveX, scaleCurveY, scaleCurveZ, 0, node->fbx.scaleKeyTimes);

    node->base.isAnimated =
        translationCurve.numKeys > 0 || rotationCurve.numKeys > 0 || scaleCurve.numKeys > 0;

    for (auto& child : node->base.children)
        PrepareAnimation(child, animStack);
}

//-------------------------------------------------------------------
/**
*/
void
NFbxNode::ExtractAnimationCurves(SceneNode* node, ufbx_node* fbxNode, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, ufbx_anim_stack* animStack)
{
    if (!node->base.isAnimated)
        return;

    ufbx_anim_layer* animLayer = animStack->layers[0];

    ufbx_prop* rotationOrder = ufbx_find_prop(&fbxNode->props, UFBX_RotationOrder);
    ufbx_rotation_order rotOrder = (ufbx_rotation_order)rotationOrder->value_int;
    ufbx_anim_prop* translationProperty = ufbx_find_anim_prop(animLayer, &fbxNode->element, UFBX_Lcl_Translation);
    ufbx_anim_prop* rotationProperty = ufbx_find_anim_prop(animLayer, &fbxNode->element, UFBX_Lcl_Rotation);
    ufbx_anim_prop* scalingProperty = ufbx_find_anim_prop(animLayer, &fbxNode->element, UFBX_Lcl_Scaling);

    ufbx_anim_curve* translationCurveX = translationProperty->anim_value->curves[0];
    ufbx_anim_curve* translationCurveY = translationProperty->anim_value->curves[1];
    ufbx_anim_curve* translationCurveZ = translationProperty->anim_value->curves[2];

    ufbx_anim_curve* rotationCurveX = rotationProperty->anim_value->curves[0];
    ufbx_anim_curve* rotationCurveY = rotationProperty->anim_value->curves[1];
    ufbx_anim_curve* rotationCurveZ = rotationProperty->anim_value->curves[2];

    ufbx_anim_curve* scaleCurveX = scalingProperty->anim_value->curves[0];
    ufbx_anim_curve* scaleCurveY = scalingProperty->anim_value->curves[1];
    ufbx_anim_curve* scaleCurveZ = scalingProperty->anim_value->curves[2];

    AnimBuilderCurve& translationCurve = node->anim.translationCurve;
    AnimBuilderCurve& rotationCurve = node->anim.rotationCurve;
    AnimBuilderCurve& scaleCurve = node->anim.scaleCurve;

    ufbx_vec3 defaultTrans = translationProperty->anim_value->default_value;
    ufbx_vec3 defaultRot = rotationProperty->anim_value->default_value;
    ufbx_vec3 defaultScale = scalingProperty->anim_value->default_value;

    translationCurve.curveType = CurveType::Translation;
    rotationCurve.curveType = CurveType::Rotation;
    scaleCurve.curveType = CurveType::Scale;

    static CoreAnimation::InfinityType::Code InfinityTranslation[] =
    {
        InfinityType::Constant,
        InfinityType::Cycle,
        InfinityType::Cycle,
        InfinityType::Cycle
    };

    struct CurveSet
    {
        ufbx_anim_curve *xCurve, *yCurve, *zCurve;
    };
    CurveSet translationSet = { translationCurveX, translationCurveY, translationCurveZ };
    CurveSet rotationSet = { rotationCurveX, rotationCurveY, rotationCurveZ }; 
    CurveSet scaleSet = { scaleCurveX, scaleCurveY, scaleCurveZ };

    // get scale
    auto extractPosScale = [](
        ufbx_vec3 defaultValue
        , const CurveSet& curves
        , const float scale
        , const Util::Set<double>& times
        , Util::Array<float>& keys
        , Util::Array<Timing::Tick>& keyTimes
        , AnimBuilderCurve& curve
        )
    {
        curve.firstKeyOffset = keys.Size();
        curve.firstTimeOffset = keyTimes.Size();
        curve.numKeys = times.Size();

        keys.Reserve(times.Size() * 3);
        keyTimes.Reserve(times.Size());
        const auto& totalTimes = times.KeysAsArray();
        for (const auto& time : totalTimes)
        {
            float values[3];
            values[0] = ufbx_evaluate_curve(curves.xCurve, time, defaultValue.x);
            values[1] = ufbx_evaluate_curve(curves.yCurve, time, defaultValue.y);
            values[2] = ufbx_evaluate_curve(curves.zCurve, time, defaultValue.z);

            keys.Append(values[0] * scale);
            keys.Append(values[1] * scale);
            keys.Append(values[2] * scale);
            keyTimes.Append(time * 1000);
        }
    };

    auto extractRotQuat = [](
        ufbx_vec3 defaultValue
        , const CurveSet& curves
        , const Util::Set<double>& times
        , ufbx_rotation_order rotationOrder
        , Util::Array<float>& keys
        , Util::Array<Timing::Tick>& keyTimes
        , AnimBuilderCurve& curve
        )
    {
        curve.firstKeyOffset = keys.Size();
        curve.firstTimeOffset = keyTimes.Size();
        curve.numKeys = times.Size();

        keys.Reserve(times.Size() * 3);
        keyTimes.Reserve(times.Size());
        const auto& totalTimes = times.KeysAsArray();
        for (const auto& time : totalTimes)
        {
            float values[3];
            values[0] = ufbx_evaluate_curve(curves.xCurve, time, defaultValue.x);
            values[1] = ufbx_evaluate_curve(curves.yCurve, time, defaultValue.y); 
            values[2] = ufbx_evaluate_curve(curves.zCurve, time, defaultValue.z);
            
            ufbx_quat quat = ufbx_euler_to_quat(ufbx_vec3 {values[0], values[1], values[2]}, rotationOrder);
            keys.Append(quat.x);
            keys.Append(quat.y);
            keys.Append(quat.z);
            keys.Append(quat.w);

            keyTimes.Append(time * 1000);
        }
    }; 

    // Extract keys
    extractPosScale(defaultTrans, translationSet, 1.0f, node->fbx.translationKeyTimes, keys, keyTimes, translationCurve);
    extractRotQuat(defaultRot, rotationSet, node->fbx.rotationKeyTimes, node->fbx.node->rotation_order, keys, keyTimes, rotationCurve);
    extractPosScale(defaultScale, scaleSet, 1.0f, node->fbx.scaleKeyTimes, keys, keyTimes, scaleCurve);
}

} // namespace ToolkitUtil