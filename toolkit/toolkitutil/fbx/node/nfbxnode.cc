//------------------------------------------------------------------------------
//  fbxnode.h.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "fbx/node/nfbxnode.h"
#include "nfbxscene.h"
#include "modelutil/clip.h"
#include "modelutil/clipevent.h"
#include "modelutil/take.h"


namespace ToolkitUtil
{

using namespace Math;
using namespace CoreAnimation;
using namespace Util;
using namespace ToolkitUtil;

__ImplementClass(ToolkitUtil::NFbxNode, 'FBNO', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NFbxNode::NFbxNode() :
    isAnimated(false),
    isRoot(false),
    isPhysics(false),
    parent(nullptr),
    type(UnknownType)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NFbxNode::~NFbxNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::Setup( FbxNode* node, const Ptr<NFbxScene>& scene )
{
    n_assert(node);
    this->fbxNode = node;
    this->scene = scene;
    this->name = node->GetName();
    if (this->name == "physics")
    {
        this->isPhysics = true;
    }
    this->ExtractTransform();
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::Discard()
{
    this->scene = nullptr;
    this->fbxNode = 0;
    this->parent = nullptr;
    this->children.Clear();
}

//------------------------------------------------------------------------------
/**
    Adds a child to this node, the childs parent has to be 0
*/
void 
NFbxNode::AddChild( const Ptr<NFbxNode>& child )
{
    n_assert(child);
    n_assert(!child->parent.isvalid());
    this->children.Append(child);
    child->SetParent(this);
}

//------------------------------------------------------------------------------
/**
    Removes child to this node, also uncouples parent
*/
void 
NFbxNode::RemoveChild( const Ptr<NFbxNode>& child )
{
    n_assert(child);
    n_assert(this == child->parent);
    this->children.EraseIndex(this->children.FindIndex(child));
    child->parent = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<NFbxNode>&
NFbxNode::GetChild( IndexT index ) const
{
    n_assert(index >= 0 && index < this->children.Size());
    return this->children[index];
}

//------------------------------------------------------------------------------
/**
*/
const IndexT 
NFbxNode::GetChildCount() const
{
    return this->children.Size();
}

//------------------------------------------------------------------------------
/**
    Generates animation clips based on node type. Mesh and transform nodes typically has a single set of curves, joint nodes typically has a tree.
*/
void 
NFbxNode::GenerateAnimationClips( const Ptr<ModelAttributes>& attributes )
{
    int animStackCount = this->fbxScene->GetSrcObjectCount<FbxAnimStack>();
    for (int animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
    {
        FbxAnimStack* animStack = this->fbxScene->GetSrcObject<FbxAnimStack>(animStackIndex);

        String takeName = animStack->GetName();
        String name = this->GetName() + "_animation";

        Util::Array<AnimBuilderCurve> curves;
        int preInfType = 0;
        int postInfType = 0;
        int span = 0;
        this->ExtractKeySpan(animStack, span);

        // having more than one frame means we have something to interpolate between
        if (span > 1)
        {
            this->ExtractAnimationCurves(animStack, curves, preInfType, postInfType, span);
            this->isAnimated = true;

            // attempt to split curves
            if (!this->SplitCurves(takeName, attributes, this->anim, curves, span))
            {
                AnimBuilderClip clip;

                int numCurves = curves.Size();
                int keyCount = 0;

                for (int curveIndex = 0; curveIndex < numCurves; curveIndex++)
                {
                    clip.AddCurve(curves[curveIndex]);
                    keyCount = Math::max(keyCount, curves[curveIndex].GetNumKeys());
                }

                clip.SetName(name);
                clip.SetNumKeys(keyCount);
                clip.SetKeyDuration(KEYS_PER_MS);
                clip.SetStartKeyIndex(0);
                clip.SetPreInfinityType(preInfType == FbxAnimCurveBase::eConstant ? InfinityType::Constant : InfinityType::Cycle);
                clip.SetPostInfinityType(postInfType == FbxAnimCurveBase::eConstant ? InfinityType::Constant : InfinityType::Cycle);

                this->anim.AddClip(clip);
            }
        }
    }
}


//------------------------------------------------------------------------------
/**
    Splits FBX take into animation clips using the animation splitter utility
*/
bool 
NFbxNode::SplitCurves( const Util::String& animStackName, const Ptr<ModelAttributes>& attributes, AnimBuilder& anim, const Util::Array<AnimBuilderCurve>& curves, int span )
{
    // split if we have attributes for this stack and resource
    if (attributes.isvalid() && attributes->HasTake(animStackName))
    {
        const Ptr<Take>& take = attributes->GetTake(animStackName);

        // we might have an empty take, in this case, just return false
        if (take->GetClips().Size() == 0)
        {
            return false;
        }

        const Util::Array<Ptr<Clip>>& clips = take->GetClips();
        for (int splitIndex = 0; splitIndex < clips.Size(); splitIndex++)
        {
            const Ptr<Clip>& split = clips[splitIndex];
            AnimBuilderClip clip;
            clip.SetName(split->GetName());
            clip.SetKeyDuration(KEYS_PER_MS);
            int clipKeyCount = split->GetEnd() - split->GetStart();
            if (clipKeyCount == 0) continue; // ignore empty clips
            clip.SetNumKeys(clipKeyCount);
            clip.SetPreInfinityType(split->GetPreInfinity() == Clip::Constant ? CoreAnimation::InfinityType::Constant : CoreAnimation::InfinityType::Cycle);
            clip.SetPostInfinityType(split->GetPostInfinity() == Clip::Constant ? CoreAnimation::InfinityType::Constant : CoreAnimation::InfinityType::Cycle);
            clip.SetStartKeyIndex(0);

            for (int curveIndex = 0; curveIndex < curves.Size(); curveIndex+=3)
            {
                const AnimBuilderCurve& curveX = curves[curveIndex];
                const AnimBuilderCurve& curveY = curves[curveIndex+1];
                const AnimBuilderCurve& curveZ = curves[curveIndex+2];

                AnimBuilderCurve splitX;
                AnimBuilderCurve splitY;
                AnimBuilderCurve splitZ;

                splitX.SetCurveType(curveX.GetCurveType());
                splitY.SetCurveType(curveY.GetCurveType());
                splitZ.SetCurveType(curveZ.GetCurveType());

                if (curveX.IsStatic())
                {
                    splitX.SetStaticKey(curveX.GetStaticKey());
                    splitX.SetStatic(true);
                    splitX.SetActive(true);
                }
                else
                {
                    splitX.ResizeKeyArray(clipKeyCount);
                    splitX.SetActive(true);
                    splitX.SetStatic(false);
                }
                if (curveY.IsStatic())
                {
                    splitY.SetStaticKey(curveY.GetStaticKey());
                    splitY.SetStatic(true);
                    splitX.SetActive(true);
                }
                else
                {
                    splitY.ResizeKeyArray(clipKeyCount);
                    splitY.SetActive(true);
                    splitY.SetStatic(false);
                }
                if (curveZ.IsStatic())
                {
                    splitZ.SetStaticKey(curveZ.GetStaticKey());
                    splitZ.SetStatic(true);
                    splitX.SetActive(true);
                }
                else
                {
                    splitZ.ResizeKeyArray(clipKeyCount);
                    splitZ.SetActive(true);
                    splitZ.SetStatic(false);
                }

                int splitIndex = 0;
                for (int keyIndex = split->GetStart(); (keyIndex < split->GetEnd()) && (keyIndex < span); keyIndex++)
                {
                    if (!splitX.IsStatic())
                    {
                        splitX.SetKey(splitIndex, curveX.GetKey(keyIndex));
                    }
                    if (!splitY.IsStatic())
                    {
                        splitY.SetKey(splitIndex, curveY.GetKey(keyIndex));
                    }
                    if (!splitZ.IsStatic())
                    {
                        splitZ.SetKey(splitIndex, curveZ.GetKey(keyIndex));
                    }
                    splitIndex++;
                }

                clip.AddCurve(splitX);
                clip.AddCurve(splitY);
                clip.AddCurve(splitZ);
            }

            const Util::Array<Ptr<ClipEvent>>& events = split->GetEvents();
            for (int eventIndex = 0; eventIndex < events.Size(); eventIndex++)
            {
                const Ptr<ClipEvent>& ev = events[eventIndex];

                CoreAnimation::AnimEvent animEvent;
                animEvent.SetCategory(ev->GetCategory());
                animEvent.SetName(ev->GetName());

                // solve the marker, in one case we have the exact number of ticks, in the other we have frames
                int marker = ev->GetMarker();
                ClipEvent::MarkerType type = ev->GetMarkerType();
                switch (type)
                {
                case ClipEvent::Ticks:
                    animEvent.SetTime(max(marker - 1 ,0) / clip.GetKeyDuration());
                    break;
                case ClipEvent::Frames:
                    animEvent.SetTime(max(marker - 1 ,0));
                    break;
                }
                clip.AddEvent(animEvent);
            }

            anim.AddClip(clip);
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
    Extract node generic transform data
*/
void 
NFbxNode::ExtractTransform()
{
    FbxMatrix localTrans = this->fbxNode->EvaluateLocalTransform();
    this->ExtractTransform(localTrans);
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxNode::ExtractTransform(const FbxMatrix& localTrans)
{
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
    this->transform = mat4(x, y, z, w);

    // calculate inverse scale
    float scaleFactor = NFbxScene::Instance()->GetScale() * 1 / float(fbxScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor());  
    
    this->rotation = vec4((scalar)rotation[0], (scalar)rotation[1], (scalar)rotation[2], (scalar)rotation[3]);
    this->position = vec4((scalar)translation[0] * scaleFactor, (scalar)translation[1] * scaleFactor, (scalar)translation[2] * scaleFactor, 1.0f);
    this->scale = vec4((scalar)scale[0], (scalar)scale[1], (scalar)scale[2], 1);
}

//-------------------------------------------------------------------
/**
*/
void 
NFbxNode::ExtractAnimationCurves( FbxAnimStack* stack, Util::Array<AnimBuilderCurve>& curves, int& postInfType, int& preInfType, int span )
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

    AnimBuilderCurve translationCurve;
    AnimBuilderCurve rotationCurve;
    AnimBuilderCurve scaleCurve;

    translationCurve.SetCurveType(CurveType::Translation);
    rotationCurve.SetCurveType(CurveType::Rotation);
    scaleCurve.SetCurveType(CurveType::Scale);

    // get scale
    float scaleFactor = NFbxScene::Instance()->GetScale() * 1 / float(fbxScene->GetGlobalSettings().GetSystemUnit().GetScaleFactor());

    // translation
    if (translationCurveX && translationCurveY && translationCurveZ)
    {
        preInfType |= translationCurveX->GetPreExtrapolation() | translationCurveY->GetPreExtrapolation() | translationCurveZ->GetPreExtrapolation();
        postInfType |= translationCurveX->GetPostExtrapolation() | translationCurveY->GetPostExtrapolation() | translationCurveZ->GetPostExtrapolation();
        int xKeys = translationCurveX->KeyGetCount();
        int yKeys = translationCurveY->KeyGetCount();
        int zKeys = translationCurveZ->KeyGetCount();
        translationCurve.ResizeKeyArray(span);
        int keyIndex;

        int xIndex = 0;
        int yIndex = 0;
        int zIndex = 0;
        for (keyIndex = 0; keyIndex < span; keyIndex++)
        {

            vec4 key = vec4(translationCurveX->EvaluateIndex(xIndex) * scaleFactor, translationCurveY->EvaluateIndex(yIndex) * scaleFactor, translationCurveZ->EvaluateIndex(zIndex) * scaleFactor, 0.0f);
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
        rotationCurve.ResizeKeyArray(span);
        int keyIndex;

        int xIndex = 0;
        int yIndex = 0;
        int zIndex = 0;
        for (keyIndex = 0; keyIndex < span; keyIndex++)
        {
            FbxAMatrix matrix; 
            matrix.SetR(FbxVector4(rotationCurveX->EvaluateIndex(xIndex), rotationCurveY->EvaluateIndex(yIndex), rotationCurveZ->EvaluateIndex(zIndex)));
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
        scaleCurve.ResizeKeyArray(span);
        int keyIndex;

        int xIndex = 0;
        int yIndex = 0;
        int zIndex = 0;
        for (keyIndex = 0; keyIndex < span; keyIndex++)
        {
            vec4 key = vec4(scaleCurveX->EvaluateIndex(xIndex), scaleCurveY->EvaluateIndex(yIndex), scaleCurveZ->EvaluateIndex(zIndex), 0.0f);
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

    curves.Append(translationCurve);
    curves.Append(rotationCurve);
    curves.Append(scaleCurve);
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxNode::ExtractKeySpan( FbxAnimStack* stack, int& span )
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

    if (translationCurveX && translationCurveY && translationCurveZ)
    {
        int numKeys = Math::max(Math::max(translationCurveX->KeyGetCount(), translationCurveY->KeyGetCount()), translationCurveZ->KeyGetCount());
        span = Math::max(span, numKeys);
    }
    if (rotationCurveX && rotationCurveY && rotationCurveZ)
    {
        int numKeys = Math::max(Math::max(rotationCurveX->KeyGetCount(), rotationCurveY->KeyGetCount()), rotationCurveZ->KeyGetCount());
        span = Math::max(span, numKeys);
    }
    if (scaleCurveX && scaleCurveY && scaleCurveZ)
    {
        int numKeys = Math::max(Math::max(scaleCurveX->KeyGetCount(), scaleCurveY->KeyGetCount()), scaleCurveZ->KeyGetCount());
        span = Math::max(span, numKeys);
    }
}



//------------------------------------------------------------------------------
/**
    Override this function to
*/
void 
NFbxNode::MergeChildren(Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes)
{
    // merge this node
    this->DoMerge(meshes);

    SizeT numChildren = this->GetChildCount();
    IndexT childIndex;
    for (childIndex = 0; childIndex < numChildren; childIndex++)
    {
        if (numChildren > this->GetChildCount())
        {
            uint diff = numChildren - this->GetChildCount();
            numChildren = this->GetChildCount();
            
            childIndex -= diff;
        }
        
        // get child
        Ptr<NFbxNode> child = this->GetChild(childIndex);

        // call child procedure
        child->MergeChildren(meshes);
    }
}


//------------------------------------------------------------------------------
/**
    Performs actual merging operation, implement this function in a subclass
*/
void 
NFbxNode::DoMerge( Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes )
{
    Ptr<NFbxNode> parent = this->GetParent();
    if (parent.isvalid())
    {
        if (parent->IsPhysics())
        {
            this->isPhysics = true;
        }
    }
}
} // namespace ToolkitUtil