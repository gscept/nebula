//------------------------------------------------------------------------------
//  transformnode.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/nodes/transformnode.h"
#include "models/nodes/transformnodeinstance.h"

namespace Models
{
__ImplementClass(Models::TransformNode, 'TRFN', Models::ModelNode);

using namespace Math;
using namespace IO;
using namespace Util;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
TransformNode::TransformNode() :
    position(0.0f, 0.0f, 0.0f),
    rotate(0.0f, 0.0f, 0.0f, 1.0f),
    scale(1.0f, 1.0f, 1.0f),
    rotatePivot(0.0f, 0.0f, 0.0f),
    scalePivot(0.0f, 0.0f, 0.0f),
    isInViewSpace(false),
    minDistance(0.0f),
    maxDistance(10000.0f),
    useLodDistances(false),
    lockedToViewer(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TransformNode::~TransformNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelNodeInstance>
TransformNode::CreateNodeInstance() const
{
    Ptr<ModelNodeInstance> newInst = (ModelNodeInstance*) TransformNodeInstance::Create();
    return newInst;
}

//------------------------------------------------------------------------------
/**
*/
Resource::State
TransformNode::GetResourceState() const
{
    return Resource::Loaded;
}

//------------------------------------------------------------------------------
/**
*/
bool 
TransformNode::ParseDataTag(const FourCC& fourCC, const Ptr<BinaryReader>& reader)
{
    bool retval = true;
    if (FourCC('POSI') == fourCC)
    {
        // position
        this->SetPosition(reader->ReadFloat4());
    }
    else if (FourCC('ROTN') == fourCC)
    {
        // rotation
        this->SetRotation(reader->ReadFloat4());
    }
    else if (FourCC('SCAL') == fourCC)
    {
        // scale
        this->SetScale(reader->ReadFloat4());
    }
    else if (FourCC('RPIV') == fourCC)
    {
        this->SetRotatePivot(reader->ReadFloat4());
    }
    else if (FourCC('SPIV') == fourCC)
    {
        this->SetScalePivot(reader->ReadFloat4());
    } 
    else if (FourCC('SVSP') == fourCC)
    {
        this->SetInViewSpace(reader->ReadBool());
    }
    else if (FourCC('SLKV') == fourCC)
    {
        this->SetLockedToViewer(reader->ReadBool());
    }
    else if (FourCC('SMID') == fourCC)
    {
        this->SetMinDistance(reader->ReadFloat());
    }
    else if (FourCC('SMAD') == fourCC)
    {
        this->SetMaxDistance(reader->ReadFloat());
    }
    else
    {
        retval = ModelNode::ParseDataTag(fourCC, reader);
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
TransformNode::OnAttachToModel(const Ptr<Model>& model)
{
    ModelNode::OnAttachToModel(model);
    if (this->parent.isvalid() && this->parent->IsA(TransformNode::RTTI))
    {
        const Ptr<TransformNode>& parentTransformNode = this->parent.cast<TransformNode>();
        if (parentTransformNode->LodDistancesUsed())
        {
            this->SetMinDistance(parentTransformNode->GetMinDistance());
            this->SetMaxDistance(parentTransformNode->GetMaxDistance());
        }        
    }
}
} // namespace Models
