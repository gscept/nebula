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
FbxToMath(const fbxsdk::FbxMatrix& matrix)
{
    Math::mat4 ret;
    ret.set(
        matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3],
        matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3],
        matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3],
        matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
FbxToMath(const fbxsdk::FbxVector4& vector)
{
    Math::vec4 ret;
    ret.set(
        vector[0]
        , vector[1]
        , vector[2]
        , vector[3]
    );
    return ret;
}


//------------------------------------------------------------------------------
/**
*/
Math::quat
FbxToMath(const fbxsdk::FbxQuaternion& quat)
{
    Math::quat ret;
    ret.set(
        quat[0]
        , quat[1]
        , quat[2]
        , quat[3]
    );
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2
FbxToMath(const fbxsdk::FbxVector2& vector)
{
    Math::vec2 ret;
    ret.set(
        vector[0]
        , vector[1]
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

    FbxVector4 translation, rotation, scale;
    translation[0] = fbxNode->local_transform.translation.x;
    translation[1] = fbxNode->local_transform.translation.y;
    translation[2] = fbxNode->local_transform.translation.z;

    rotation[0] = fbxNode->local_transform.rotation.x;
    rotation[1] = fbxNode->local_transform.rotation.y;
    rotation[2] = fbxNode->local_transform.rotation.z;

    scale[0] = fbxNode->local_transform.scale.x;
    scale[1] = fbxNode->local_transform.scale.y;
    scale[2] = fbxNode->local_transform.scale.z;

    translation.FixIncorrectValue();
    rotation.FixIncorrectValue();
    scale.FixIncorrectValue();
    node->base.rotation = Math::quatyawpitchroll(rotation[1], rotation[0], rotation[2]);
    node->base.translation = xyz(FbxToMath(translation)) * AdjustedScale;
    node->base.scale = xyz(FbxToMath(scale)) * AdjustedScale;

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
        ufbx_node* fbxNode = node->fbx.node;
        ExtractAnimationCurves(node, fbxNode, keys, keyTimes, animStack);
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
                times.Add(fbxCurveX->keyframes.data[keyIndex].time);
            }
        }
        if (fbxCurveY != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveY->keyframes.count; keyIndex++)
            {
                times.Add(fbxCurveY->keyframes.data[keyIndex].time);
            }
        }
        if (fbxCurveZ != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveZ->keyframes.count; keyIndex++)
            {
                times.Add(fbxCurveZ->keyframes.data[keyIndex].time);
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

    ufbx_node* fbxNode = node->fbx.node;
    ufbx_anim_layer* animLayer = animStack->layers[0];

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

    Math::vec3 defaultTrans = node->base.translation * (1.0f / AdjustedScale);
    Math::quat defaultRot = node->base.rotation;
    Math::vec3 defaultScale = node->base.scale * (1.0f / AdjustedScale);

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
        Math::vec3 defaultValue
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
        int lastX = 0, lastY = 0, lastZ = 0;
        for (const auto& time : totalTimes)
        {
            float values[3];

            if (curves.xCurve != nullptr)
                values[0] = ufbx_evaluate_curve(curves.xCurve, time, lastX);
            else
                values[0] = defaultValue[0];

            if (curves.yCurve != nullptr)
                values[1] =  ufbx_evaluate_curve(curves.yCurve, time, lastX);
            else
                values[1] = defaultValue[1];

            if (curves.zCurve != nullptr)
                values[2] =  ufbx_evaluate_curve(curves.zCurve, time, lastX);
            else
                values[2] = defaultValue[2];

            keys.Append(values[0] * scale);
            keys.Append(values[1] * scale);
            keys.Append(values[2] * scale);
            keyTimes.Append(time);
        }
    };

    auto extractRotQuat = [](
        Math::quat defaultValue
        , const CurveSet& curves
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
        int lastX = 0, lastY = 0, lastZ = 0;
        for (const auto& time : totalTimes)
        {
            float values[3];

            Math::vec3 euler = Math::to_euler(defaultValue);

            if (curves.xCurve != nullptr)
                values[0] =  ufbx_evaluate_curve(curves.xCurve, time, lastX);
            else
                values[0] = euler.x;

            if (curves.yCurve != nullptr)
                values[1] =  ufbx_evaluate_curve(curves.yCurve, time, lastX);
            else
                values[1] = euler.y;

            if (curves.zCurve != nullptr)
                values[2] =  ufbx_evaluate_curve(curves.zCurve, time, lastX);
            else
                values[2] = euler.z;
            

            Math::quat quat = quatyawpitchroll(Math::deg2rad(values[1]), Math::deg2rad(values[0]), Math::deg2rad(values[2]));
            keys.Append(quat.x);
            keys.Append(quat.y);
            keys.Append(quat.z);
            keys.Append(quat.w);

            keyTimes.Append(time);
        }
    }; 

    // Extract keys
    extractPosScale(defaultTrans, translationSet, AdjustedScale, node->fbx.translationKeyTimes, keys, keyTimes, translationCurve);
    extractRotQuat(defaultRot, rotationSet, node->fbx.rotationKeyTimes, keys, keyTimes, rotationCurve);
    extractPosScale(defaultScale, scaleSet, 1.0f, node->fbx.scaleKeyTimes, keys, keyTimes, scaleCurve);
}

} // namespace ToolkitUtil