#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxJointNode
    
    Wraps an FBX skeleton node as a Nebula-style node
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "fbx/node/nfbxnode.h"
namespace ToolkitUtil
{
class NFbxJointNode : public NFbxNode
{
    __DeclareClass(NFbxJointNode);
public:
    /// constructor
    NFbxJointNode();
    /// destructor
    virtual ~NFbxJointNode();

    /// sets up from fbx node
    void Setup(FbxNode* node, const Ptr<NFbxScene>& scene, int index, FbxPose* bindpose);

    /// returns true if joint is root
    const bool IsRoot() const;

    /// returns FBX skeleton
    FbxSkeleton* GetSkeleton() const;

    /// sets up joint matrix from bind pose
    void SetupFromCluster(FbxCluster* cluster);
    /// goes through children and converts transform to local transform
    void ConvertJointsToLocal();

    /// gets the parent joint index
    IndexT GetParentJoint() const;
    /// gets the joint index
    IndexT GetJointIndex() const;

private:
    friend class NFbxScene;

    /// recursively traverses skeleton hierarchy to construct animation clips
    void ExtractAnimationCurves(FbxAnimStack* stack, Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int& postInfType, int& preInfType, int span);
    /// recursively traverses skeleton hierarchy to find the total keyspan
    void ExtractKeySpan(FbxAnimStack* stack, int& span);
    /// recursively traverses children and multiplies child matrices with current
    void RecursiveConvertToLocal(const Ptr<NFbxJointNode>& parent);
    

    IndexT                      parentIndex;
    IndexT                      jointIndex;
    FbxSkeleton*                joint;
    FbxCluster*                 cluster;
    FbxMatrix                   globalMatrix;

    bool                        matrixIsGlobal;
    bool                        isSkeletonRoot;
}; 


//------------------------------------------------------------------------------
/**
*/
inline const bool 
NFbxJointNode::IsRoot() const
{
    return this->isSkeletonRoot;
}

//------------------------------------------------------------------------------
/**
*/
inline FbxSkeleton* 
NFbxJointNode::GetSkeleton() const
{
    return this->joint;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT 
NFbxJointNode::GetParentJoint() const
{
    return this->parentIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT 
NFbxJointNode::GetJointIndex() const
{
    return this->jointIndex;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------