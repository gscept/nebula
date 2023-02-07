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
        matrix.mData[0][0], matrix.mData[0][1], matrix.mData[0][2], matrix.mData[0][3],
        matrix.mData[1][0], matrix.mData[1][1], matrix.mData[1][2], matrix.mData[1][3],
        matrix.mData[2][0], matrix.mData[2][1], matrix.mData[2][2], matrix.mData[2][3],
        matrix.mData[3][0], matrix.mData[3][1], matrix.mData[3][2], matrix.mData[3][3]);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
FbxToMath(const fbxsdk::FbxVector4& vector)
{
    Math::vec4 ret;
    ret.set(vector.mData[0], vector.mData[1], vector.mData[2], vector.mData[3]);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::Setup(SceneNode* node, FbxNode* fbxNode)
{
    node->base.name = fbxNode->GetName();
    if (node->base.name == "physics")
    {
        node->base.isPhysics = true;
    }
    FbxMatrix localTrans = fbxNode->EvaluateLocalTransform();

    FbxQuaternion rotation;
    FbxVector4 translation;
    FbxVector4 scale;
    FbxVector4 shear;
    double sign;

    // decompose elements
    localTrans.GetElements(translation, rotation, shear, scale, sign);

    // decompose matrix into rows
    FbxVector4 xRow = localTrans.GetRow(0);
    vec4 x = vec4((scalar)xRow[0], (scalar)xRow[1], (scalar)xRow[2], (scalar)xRow[3]);
    FbxVector4 yRow = localTrans.GetRow(1);
    vec4 y = vec4((scalar)yRow[0], (scalar)yRow[1], (scalar)yRow[2], (scalar)yRow[3]);
    FbxVector4 zRow = localTrans.GetRow(2);
    vec4 z = vec4((scalar)zRow[0], (scalar)zRow[1], (scalar)zRow[2], (scalar)zRow[3]);
    FbxVector4 wRow = localTrans.GetRow(3);
    vec4 w = vec4((scalar)wRow[0], (scalar)wRow[1], (scalar)wRow[2], (scalar)wRow[3]);

    // construct nebula matrix from rows    
    node->base.transform = mat4(x, y, z, w);

    // calculate inverse scale
    float scaleFactor = AdjustedScale;

    node->base.rotation = vec4((scalar)rotation[0], (scalar)rotation[1], (scalar)rotation[2], (scalar)rotation[3]);
    node->base.position = vec3((scalar)translation[0] * scaleFactor, (scalar)translation[1] * scaleFactor, (scalar)translation[2] * scaleFactor);
    node->base.scale = vec3((scalar)scale[0], (scalar)scale[1], (scalar)scale[2]);
}

//------------------------------------------------------------------------------
/**
    Generates animation clips based on node type. Mesh and transform nodes typically has a single set of curves, joint nodes typically has a tree.
*/
void
NFbxNode::ExtractAnimation(SceneNode* node, FbxNode* fbxNode, FbxAnimStack* animStack)
{
    ExtractAnimationCurves(node, fbxNode, animStack);

    // Having more than one animation key means we have an animation
    if (node->anim.span > 1)
    {
        String takeName = animStack->GetName();
        node->base.isAnimated = true;
        node->anim.take = takeName;
    }
}

//-------------------------------------------------------------------
/**
*/
void
NFbxNode::ExtractAnimationCurves(SceneNode* node, FbxNode* fbxNode, FbxAnimStack* stack)
{
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

    node->anim.span = 0;
    if (translationCurveX && translationCurveY && translationCurveZ)
    {
        int numKeys = Math::max(Math::max(translationCurveX->KeyGetCount(), translationCurveY->KeyGetCount()), translationCurveZ->KeyGetCount());
        node->anim.span = Math::max(node->anim.span, numKeys);
    }
    if (rotationCurveX && rotationCurveY && rotationCurveZ)
    {
        int numKeys = Math::max(Math::max(rotationCurveX->KeyGetCount(), rotationCurveY->KeyGetCount()), rotationCurveZ->KeyGetCount());
        node->anim.span = Math::max(node->anim.span, numKeys);
    }
    if (scaleCurveX && scaleCurveY && scaleCurveZ)
    {
        int numKeys = Math::max(Math::max(scaleCurveX->KeyGetCount(), scaleCurveY->KeyGetCount()), scaleCurveZ->KeyGetCount());
        node->anim.span = Math::max(node->anim.span, numKeys);
    }

    AnimBuilderCurve& translationCurve = node->anim.translationCurve;
    AnimBuilderCurve& rotationCurve = node->anim.rotationCurve;
    AnimBuilderCurve& scaleCurve = node->anim.scaleCurve;

    translationCurve.SetCurveType(CurveType::Translation);
    rotationCurve.SetCurveType(CurveType::Rotation);
    scaleCurve.SetCurveType(CurveType::Scale);

    // get scale
    float scaleFactor = AdjustedScale;

    // translation
    int preInfType = 0, postInfType = 0;
    if (translationCurveX && translationCurveY && translationCurveZ)
    {
        preInfType |= translationCurveX->GetPreExtrapolation() | translationCurveY->GetPreExtrapolation() | translationCurveZ->GetPreExtrapolation();
        postInfType |= translationCurveX->GetPostExtrapolation() | translationCurveY->GetPostExtrapolation() | translationCurveZ->GetPostExtrapolation();
        int xKeys = translationCurveX->KeyGetCount();
        int yKeys = translationCurveY->KeyGetCount();
        int zKeys = translationCurveZ->KeyGetCount();
        translationCurve.ResizeKeyArray(node->anim.span);
        int keyIndex;

        int xIndex = 0;
        int yIndex = 0;
        int zIndex = 0;
        for (keyIndex = 0; keyIndex < node->anim.span; keyIndex++)
        {
            vec4 key = vec4(translationCurveX->KeyGetValue(xIndex) * scaleFactor, translationCurveY->KeyGetValue(yIndex) * scaleFactor, translationCurveZ->KeyGetValue(zIndex) * scaleFactor, 0.0f);
            translationCurve.SetKey(keyIndex, key);
            if (keyIndex+1 < xKeys)
            {
                xIndex++;
            }
            if (keyIndex+1 < yKeys)
            {
                yIndex++;
            }
            if (keyIndex+1 < zKeys)
            {
                zIndex++;
            }
        }
        translationCurve.SetStatic(false);
        translationCurve.SetActive(true);
    }
    else
    {
        translationCurve.SetFirstKeyIndex(0);
        translationCurve.SetStatic(true);
        translationCurve.SetActive(true);
        vec4 key = vec4((float)fbxNode->LclTranslation.Get()[0] * scaleFactor, (float)fbxNode->LclTranslation.Get()[1] * scaleFactor, (float)fbxNode->LclTranslation.Get()[2] * scaleFactor, 0.0f);
        translationCurve.SetStaticKey(key);
    }

    // rotation
    if (rotationCurveX && rotationCurveY && rotationCurveZ)
    {
        preInfType |= rotationCurveX->GetPreExtrapolation() | rotationCurveY->GetPreExtrapolation() | rotationCurveZ->GetPreExtrapolation();
        postInfType |= rotationCurveX->GetPostExtrapolation() | rotationCurveY->GetPostExtrapolation() | rotationCurveZ->GetPostExtrapolation();
        int xKeys = rotationCurveX->KeyGetCount();
        int yKeys = rotationCurveY->KeyGetCount();
        int zKeys = rotationCurveZ->KeyGetCount();
        rotationCurve.ResizeKeyArray(node->anim.span);
        int keyIndex;

        int xIndex = 0;
        int yIndex = 0;
        int zIndex = 0;
        for (keyIndex = 0; keyIndex < node->anim.span; keyIndex++)
        {
            FbxAMatrix matrix; 
            matrix.SetR(FbxVector4(rotationCurveX->KeyGetValue(xIndex), rotationCurveY->KeyGetValue(yIndex), rotationCurveZ->KeyGetValue(zIndex)));
            FbxQuaternion quat = matrix.GetQ();
            vec4 key = vec4((scalar)quat[0], (scalar)quat[1], (scalar)quat[2], (scalar)quat[3]);
            rotationCurve.SetKey(keyIndex, key);

            if (keyIndex+1 < xKeys)
            {
                xIndex++;
            }
            if (keyIndex+1 < yKeys)
            {
                yIndex++;
            }
            if (keyIndex+1 < zKeys)
            {
                zIndex++;
            }
        }

        rotationCurve.SetStatic(false);
        rotationCurve.SetActive(true);
    }
    else
    {
        rotationCurve.SetFirstKeyIndex(0);
        rotationCurve.SetStatic(true);
        rotationCurve.SetActive(true);
        FbxAMatrix matrix;
        matrix.SetR(FbxVector4(fbxNode->LclRotation.Get()[0], fbxNode->LclRotation.Get()[1], fbxNode->LclRotation.Get()[2]));
        FbxQuaternion quat = matrix.GetQ();
        vec4 key = vec4((scalar)quat[0], (scalar)quat[1], (scalar)quat[2], (scalar)quat[3]);
        rotationCurve.SetStaticKey(key);
    }

    //scaling
    if (scaleCurveX && scaleCurveY && scaleCurveZ)
    {
        preInfType |= scaleCurveX->GetPreExtrapolation() | scaleCurveY->GetPreExtrapolation() | scaleCurveZ->GetPreExtrapolation();
        postInfType |= scaleCurveX->GetPostExtrapolation() | scaleCurveY->GetPostExtrapolation() | scaleCurveZ->GetPostExtrapolation();
        int xKeys = scaleCurveX->KeyGetCount();
        int yKeys = scaleCurveY->KeyGetCount();
        int zKeys = scaleCurveZ->KeyGetCount();
        scaleCurve.ResizeKeyArray(node->anim.span);
        int keyIndex;

        int xIndex = 0;
        int yIndex = 0;
        int zIndex = 0;
        for (keyIndex = 0; keyIndex < node->anim.span; keyIndex++)
        {
            vec4 key = vec4(scaleCurveX->KeyGetValue(xIndex), scaleCurveY->KeyGetValue(yIndex), scaleCurveZ->KeyGetValue(zIndex), 0.0f);
            scaleCurve.SetKey(keyIndex, key);

            if (keyIndex+1 < xKeys)
            {
                xIndex++;
            }
            if (keyIndex+1 < yKeys)
            {
                yIndex++;
            }
            if (keyIndex+1 < zKeys)
            {
                zIndex++;
            }
        }
        scaleCurve.SetStatic(false);
        scaleCurve.SetActive(true);
    }
    else
    {
        scaleCurve.SetFirstKeyIndex(0);
        scaleCurve.SetStatic(true);
        scaleCurve.SetActive(true);
        vec4 key = vec4((float)fbxNode->LclScaling.Get()[0], (float)fbxNode->LclScaling.Get()[1], (float)fbxNode->LclScaling.Get()[2], 0.0f);
        scaleCurve.SetStaticKey(key);
    }

    node->anim.preInfinity = preInfType ? InfinityType::Constant : InfinityType::Cycle;
    node->anim.postInfinity = postInfType ? InfinityType::Constant : InfinityType::Cycle;
}

} // namespace ToolkitUtil