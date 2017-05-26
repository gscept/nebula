//------------------------------------------------------------------------------
//  materialshapenode.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/nodes/shapenode.h"
#include "models/nodes/shapenodeinstance.h"
#include "resources/resourcemanager.h"
#include "coregraphics/renderdevice.h"

namespace Models
{
__ImplementClass(Models::ShapeNode, 'SPND', Models::StateNode);


using namespace Util;
using namespace IO;
using namespace Resources;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
ShapeNode::ShapeNode() :
    primGroupIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ShapeNode::~ShapeNode()
{
    n_assert(!this->managedMesh.isvalid());
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelNodeInstance>
ShapeNode::CreateNodeInstance() const
{
    Ptr<ModelNodeInstance> newInst = (ModelNodeInstance*) ShapeNodeInstance::Create();
    return newInst;
}

//------------------------------------------------------------------------------
/**
*/
bool
ShapeNode::ParseDataTag(const FourCC& fourCC, const Ptr<BinaryReader>& reader)
{
    bool retval = true;
    if (FourCC('MESH') == fourCC)
    {
        // Mesh
        this->SetMeshResourceId(reader->ReadString());
    }
    else if (FourCC('PGRI') == fourCC)
    {
        // PrimitiveGroupIndex
        this->SetPrimitiveGroupIndex(reader->ReadInt());
    }
    else
    {
        retval = StateNode::ParseDataTag(fourCC, reader);
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
Resource::State
ShapeNode::GetResourceState() const
{
    Resource::State state = StateNode::GetResourceState();
    if (this->managedMesh->GetState() > state)
    {
        state = this->managedMesh->GetState();
    }
    return state;
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeNode::LoadResources(bool sync)
{    
    n_assert(this->meshResId.IsValid());
    n_assert(InvalidIndex != this->primGroupIndex);
    
    if (!this->managedMesh.isvalid())
    {
        // create a managed mesh resource    
        this->managedMesh = ResourceManager::Instance()->CreateManagedResource(Mesh::RTTI, this->meshResId, this->resourceLoader, sync).downcast<ManagedMesh>();         
    }
    n_assert(this->managedMesh.isvalid());

	// call parent class
	StateNode::LoadResources(sync);
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeNode::UnloadResources()
{
    n_assert(this->managedMesh.isvalid());

    // discard managed resource
    ResourceManager::Instance()->DiscardManagedResource(this->managedMesh.upcast<ManagedResource>());
    this->managedMesh = 0;

    // call parent class
    StateNode::UnloadResources();
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeNode::ApplySharedState(IndexT frameIndex)
{
    n_assert(this->managedMesh.isvalid());
    n_assert(this->primGroupIndex != InvalidIndex);
    
    // first call parent class
    StateNode::ApplySharedState(frameIndex);

    // setup the render device to render our mesh (maybe placeholder mesh
    // if asynchronous resource loading hasn't finished yet)
    const Ptr<Mesh>& mesh = this->managedMesh->GetMesh();    
    if (this->managedMesh->GetState() == Resource::Loaded)
    {
        mesh->ApplyPrimitives(this->primGroupIndex);        
    }
    else
    {
        mesh->ApplyPrimitives(0);        
    }


    // @todo: visible ShapeNodeInstances must provide render feedback (screen size)
    // to our managed mesh!
}

} // namespace Models