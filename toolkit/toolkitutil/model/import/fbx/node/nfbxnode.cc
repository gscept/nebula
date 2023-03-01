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
NFbxNode::Setup(SceneNode* node, SceneNode* parent, FbxNode* fbxNode)
{
    node->base.name = fbxNode->GetName();
    if (node->base.name == "physics")
    {
        node->base.isPhysics = true;
    }

    FbxVector4 translation, rotation, scale;
    FbxAnimCurveNode* translationCurve = fbxNode->LclTranslation.GetCurveNode();
    if (translationCurve)
    {
        translation[0] = translationCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_X, 0.0f);
        translation[1] = translationCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_Y, 0.0f);
        translation[2] = translationCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_Z, 0.0f);
    }
    else
        translation = fbxNode->LclTranslation.Get();

    FbxAnimCurveNode* rotationCurve = fbxNode->LclRotation.GetCurveNode();
    if (rotationCurve)
    {
        rotation[0] = rotationCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_X, 0.0f);
        rotation[1] = rotationCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_Y, 0.0f);
        rotation[2] = rotationCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_Z, 0.0f);
    }
    else
        rotation = fbxNode->LclRotation.Get();

    FbxAnimCurveNode* scaleCurve = fbxNode->LclScaling.GetCurveNode();
    if (scaleCurve)
    {
        scale[0] = scaleCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_X, 0.0f);
        scale[1] = scaleCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_Y, 0.0f);
        scale[2] = scaleCurve->GetChannelValue(FBXSDK_CURVENODE_COMPONENT_Z, 0.0f);
    }
    else
        scale = fbxNode->LclScaling.Get();

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
NFbxNode::ExtractAnimation(SceneNode* node, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, FbxAnimStack* animStack)
{
    // Run recursive function to collect animation curves this node hierarchy
    std::function<void(SceneNode*)> recursiveExtract = [&](SceneNode* node)
    {
        FbxNode* fbxNode = node->fbx.node;
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
CountKeys(FbxAnimCurve* fbxCurveX, FbxAnimCurve* fbxCurveY, FbxAnimCurve* fbxCurveZ, Timing::Tick sampleRate, Util::Set<FbxTime>& times)
{
    // If we have a sample rate, extract keys based on sample frequency
    if (sampleRate != 0)
    {
        FbxTimeSpan totalSpan;
        if (fbxCurveX != nullptr)
        {
            FbxTimeSpan localSpan;
            fbxCurveX->GetTimeInterval(localSpan);
            totalSpan.UnionAssignment(localSpan);
        }
        if (fbxCurveY != nullptr)
        {
            FbxTimeSpan localSpan;
            fbxCurveY->GetTimeInterval(localSpan);
            totalSpan.UnionAssignment(localSpan);
        }
        if (fbxCurveZ != nullptr)
        {
            FbxTimeSpan localSpan;
            fbxCurveZ->GetTimeInterval(localSpan);
            totalSpan.UnionAssignment(localSpan);
        }
        FbxTime start = totalSpan.GetStart();
        FbxTime keyEvaluationPoint = start;
        while (keyEvaluationPoint < totalSpan.GetStop())
        {
            auto ms = keyEvaluationPoint.GetMilliSeconds() + 8;
            keyEvaluationPoint.SetMilliSeconds(ms);
            times.Add(keyEvaluationPoint);
        }
    }
    else
    {
        if (fbxCurveX != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveX->KeyGetCount(); keyIndex++)
            {
                times.Add(fbxCurveX->KeyGetTime(keyIndex));
            }
        }
        if (fbxCurveY != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveY->KeyGetCount(); keyIndex++)
            {
                times.Add(fbxCurveY->KeyGetTime(keyIndex));
            }
        }
        if (fbxCurveZ != nullptr)
        {
            for (IndexT keyIndex = 0; keyIndex < fbxCurveZ->KeyGetCount(); keyIndex++)
            {
                times.Add(fbxCurveZ->KeyGetTime(keyIndex));
            }
        }
    }
    return times.Size();
};

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::PrepareAnimation(SceneNode* node, FbxAnimStack* animStack)
{
    // we only need the base (which contains the sum of all layers)
    FbxNode* fbxNode = node->fbx.node;
    FbxAnimLayer* animLayer = animStack->GetSrcObject<FbxAnimLayer>(0);

    FbxAnimCurve* translationCurveX = fbxNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* translationCurveY = fbxNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* translationCurveZ = fbxNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

    FbxAnimCurve* rotationCurveX = fbxNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* rotationCurveY = fbxNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* rotationCurveZ = fbxNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

    FbxAnimCurve* scaleCurveX = fbxNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* scaleCurveY = fbxNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* scaleCurveZ = fbxNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

    AnimBuilderCurve& translationCurve = node->anim.translationCurve;
    AnimBuilderCurve& rotationCurve = node->anim.rotationCurve;
    AnimBuilderCurve& scaleCurve = node->anim.scaleCurve;

    translationCurve.curveType = CurveType::Translation;
    rotationCurve.curveType = CurveType::Rotation;
    scaleCurve.curveType = CurveType::Scale;

    FbxAnimCurve* translations[] = { translationCurveX, translationCurveY, translationCurveZ };
    FbxAnimCurve* rotations[] = { rotationCurveX, rotationCurveY, rotationCurveZ };
    FbxAnimCurve* scalings[] = { scaleCurveX, scaleCurveY, scaleCurveZ };

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
NFbxNode::ExtractAnimationCurves(SceneNode* node, FbxNode* fbxNode, Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, FbxAnimStack* stack)
{
    if (!node->base.isAnimated)
        return;

    // we only need the base (which contains the sum of all layers)
    FbxAnimLayer* animLayer = stack->GetSrcObject<FbxAnimLayer>(0);

    FbxAnimCurve* translationCurveX = fbxNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* translationCurveY = fbxNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* translationCurveZ = fbxNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

    FbxAnimCurve* rotationCurveX = fbxNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* rotationCurveY = fbxNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* rotationCurveZ = fbxNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

    FbxAnimCurve* scaleCurveX = fbxNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
    FbxAnimCurve* scaleCurveY = fbxNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
    FbxAnimCurve* scaleCurveZ = fbxNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

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
        FbxAnimCurve *xCurve, *yCurve, *zCurve;
    };
    CurveSet translationSet = { translationCurveX, translationCurveY, translationCurveZ };
    CurveSet rotationSet = { rotationCurveX, rotationCurveY, rotationCurveZ };
    CurveSet scaleSet = { scaleCurveX, scaleCurveY, scaleCurveZ };

    // get scale
    auto extractPosScale = [](
        Math::vec3 defaultValue
        , const CurveSet& curves
        , const float scale
        , const Util::Set<FbxTime>& times
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
                values[0] = curves.xCurve->Evaluate(time, &lastX);
            else
                values[0] = defaultValue[0];

            if (curves.yCurve != nullptr)
                values[1] = curves.yCurve->Evaluate(time, &lastY);
            else
                values[1] = defaultValue[1];

            if (curves.zCurve != nullptr)
                values[2] = curves.zCurve->Evaluate(time, &lastZ);
            else
                values[2] = defaultValue[2];

            keys.Append(values[0] * scale);
            keys.Append(values[1] * scale);
            keys.Append(values[2] * scale);
            keyTimes.Append(time.GetMilliSeconds());
        }
    };

    auto extractRotQuat = [](
        Math::quat defaultValue
        , const CurveSet& curves
        , const Util::Set<FbxTime>& times
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
                values[0] = curves.xCurve->Evaluate(time, &lastX);
            else
                values[0] = euler.x;

            if (curves.yCurve != nullptr)
                values[1] = curves.yCurve->Evaluate(time, &lastY);
            else
                values[1] = euler.y;

            if (curves.zCurve != nullptr)
                values[2] = curves.zCurve->Evaluate(time, &lastZ);
            else
                values[2] = euler.z;
            

            Math::quat quat = quatyawpitchroll(Math::deg2rad(values[1]), Math::deg2rad(values[0]), Math::deg2rad(values[2]));
            keys.Append(quat.x);
            keys.Append(quat.y);
            keys.Append(quat.z);
            keys.Append(quat.w);

            keyTimes.Append(time.GetMilliSeconds());
        }
    }; 

    // Extract keys
    extractPosScale(defaultTrans, translationSet, AdjustedScale, node->fbx.translationKeyTimes, keys, keyTimes, translationCurve);
    extractRotQuat(defaultRot, rotationSet, node->fbx.rotationKeyTimes, keys, keyTimes, rotationCurve);
    extractPosScale(defaultScale, scaleSet, AdjustedScale, node->fbx.scaleKeyTimes, keys, keyTimes, scaleCurve);
}

} // namespace ToolkitUtil