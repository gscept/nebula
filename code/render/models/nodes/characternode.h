#pragma once
//------------------------------------------------------------------------------
/**
    @class Characters::CharacterNode
  
    The CharacterNode class wraps a Character object into a ModelNode
    for rendering.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/  
#include "models/nodes/transformnode.h"
//#include "characters/character.h"
#include "models/model.h"
#include "coregraphics/shader.h"

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

    /// called once when all pending resource have been loaded
    void OnFinishedLoading();
	/// called when resources have loaded
	void OnResourcesLoaded();
    /// parse data tag (called by loader code)
    bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	/// discard node
	void Discard();

    /// get the owned character object
    //const Ptr<Character>& GetCharacter() const;
    /// get the character's managed animation resource
    const Resources::ResourceName& GetAnimResource() const;

    /// get the character's animation resource
    const Resources::ResourceId GetAnimationResourceId() const;

	struct Instance : public TransformNode::Instance
	{
		Ids::Id32 characterId;
		IndexT updateFrame;
		bool updateThisFrame;
		Util::HashTable<Util::StringAtom, Models::ModelNode::Instance*, 8> activeSkinInstances;
		Util::FixedArray<Math::matrix44> joints;

		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;

	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }

private:
    /// recursively create model node instance and child model node instances
    //virtual Ptr<Models::ModelNodeInstance> RecurseCreateNodeInstanceHierarchy(const Ptr<Models::ModelInstance>& modelInst, const Ptr<Models::ModelNodeInstance>& parentNodeInst=0);

protected:

    Resources::ResourceName animResId;
    Resources::ResourceName variationResId;
    //Ptr<Character> character;
	Util::StringAtom tag;
    Resources::ResourceId managedAnimResource;
    Resources::ResourceId managedVariationResource;

	CoreGraphics::ShaderId sharedShader;
	CoreGraphics::ConstantBufferId cbo;
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
inline const Ptr<Character>&
CharacterNode::GetCharacter() const
{
    return this->character;
}
*/

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

ModelNodeInstanceCreator(CharacterNode)

//------------------------------------------------------------------------------
/**
*/
inline void
CharacterNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	ModelNode::Instance::Setup(node, parent);
}

} // namespace Characters
//------------------------------------------------------------------------------
  
