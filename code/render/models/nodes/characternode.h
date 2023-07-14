#pragma once
//------------------------------------------------------------------------------
/**
    @class Characters::CharacterNode
  
    The CharacterNode class wraps a Character object into a ModelNode
    for rendering.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/  
#include "models/nodes/transformnode.h"
#include "coregraphics/shader.h"
#include "coregraphics/buffer.h"

//------------------------------------------------------------------------------
namespace Models
{
class CharacterNode : public Models::TransformNode
{
public:
    /// constructor
    CharacterNode();
    /// destructor
    virtual ~CharacterNode();

    /// called when resources have loaded
    void OnResourcesLoaded();
    /// parse data tag (called by loader code)
    bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;

    /// discard node
    void Discard();

    /// get the owned character object
    //const Ptr<Character>& GetCharacter() const;
    /// get the character's managed animation resource
    const Resources::ResourceName& GetAnimResource() const;

    /// get the character's animation resource
    const Resources::ResourceId GetAnimationResourceId() const;

    /// character nodes should not create the hierarchy implicitly
    bool GetImplicitHierarchyActivation() const override;

private:
    /// recursively create model node instance and child model node instances
    //virtual Ptr<Models::ModelNodeInstance> RecurseCreateNodeInstanceHierarchy(const Ptr<Models::ModelInstance>& modelInst, const Ptr<Models::ModelNodeInstance>& parentNodeInst=0);

protected:

    Resources::ResourceName animResId;
    IndexT animIndex;
    Resources::ResourceName skeletonResId;
    IndexT skeletonIndex;
    Resources::ResourceName variationResId;
    Util::StringAtom tag;
    Resources::ResourceId managedAnimResource;
    Resources::ResourceId managedVariationResource;

    CoreGraphics::ShaderId sharedShader;
    CoreGraphics::BufferId cbo;
    IndexT cboIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
CharacterNode::GetAnimResource() const
{
    return this->animResId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId 
CharacterNode::GetAnimationResourceId() const
{
    return this->managedAnimResource;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
CharacterNode::GetImplicitHierarchyActivation() const
{
    return true;
}


} // namespace Characters
//------------------------------------------------------------------------------
  
