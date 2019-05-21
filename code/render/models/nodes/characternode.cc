//------------------------------------------------------------------------------
//  characternode.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "characternode.h"
#include "resources/resourcemanager.h"
#include "models/modelpool.h"
#include "coregraphics/shaderserver.h"
//#include "characterjointmask.h"

namespace Models
{

using namespace Models;
using namespace Util;
using namespace IO;
//using namespace CoreAnimation;
using namespace Resources;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
CharacterNode::CharacterNode()
{
	this->type = CharacterNodeType;
}

//------------------------------------------------------------------------------
/**
*/
CharacterNode::~CharacterNode()
{
    n_assert(this->managedAnimResource == Ids::InvalidId64);
    n_assert(this->managedVariationResource == Ids::InvalidId64);
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterNode::Discard()
{
	//n_assert(this->character.isvalid());

	// discard character object
	// this->character->Discard();
	//this->character = 0;
}

//------------------------------------------------------------------------------
/**
    Called when all resources of this Model are loaded. We need to setup
    the animation and variation libraries once this has happened.
*/
void
CharacterNode::OnFinishedLoading()
{
	// setup all skinlist -> model node bindings
	IndexT skinIndex;
	for (skinIndex = 0; skinIndex < this->children.Size(); skinIndex++)
	{
		this->skinNodes.Add(this->children[skinIndex]->name, skinIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterNode::OnResourcesLoaded()
{
	n_assert(this->managedAnimResource != Ids::InvalidId64);

	// setup the character's animation library
	//this->character->AnimationLibrary().Setup(this->managedAnimResource->GetAnimResource());

	if (this->managedVariationResource != Ids::InvalidId64)
	{
		// setup the character's variation library
		//this->character->VariationLibrary().Setup(this->managedVariationResource->GetAnimResource(), this->character->Skeleton());
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
CharacterNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader)
{
    bool retval = true;
	this->tag = tag;

    if (FourCC('ANIM') == fourcc)
    {
        // Animation
        this->animResId = reader->ReadString();
    }
	else if (FourCC('SKEL') == fourcc)
	{
		this->skeletonResId = reader->ReadString();
	}
	else if (FourCC('NJMS') == fourcc)
	{
		SizeT numMasks = reader->ReadInt();
		//this->character->Skeleton().ReserveMasks(numMasks);
	}
	else if (FourCC('JOMS') == fourcc)
	{
		StringAtom maskName = reader->ReadString();
		SizeT num = reader->ReadInt();
		IndexT i;
		for (i = 0; i < num; i++)
		{
			reader->ReadFloat();
		}
		/*
		CharacterJointMask mask;
		StringAtom maskName = reader->ReadString();
		SizeT num = reader->ReadInt();
		Util::FixedArray<scalar> weights;
		weights.Resize(num);
		IndexT i;
		for (i = 0; i < num; i++)
		{
			weights[i] = reader->ReadFloat();
		}
		mask.SetName(maskName);
		mask.SetWeights(weights);
		this->character->Skeleton().AddJointMask(mask);
		*/
	}
    else if (FourCC('VART') == fourcc)
    {
        // variation resource name
        this->variationResId = reader->ReadString();

		// send to loader
		// loader->pendingResources.Append(reader->ReadString());
    }
    else if (FourCC('NSKL') == fourcc)
    {
		this->skinLists.Resize(reader->ReadInt());
		this->skinListIndex = 0;
    }
    else if (FourCC('SKNL') == fourcc)
    {
		const Util::StringAtom skinListName = reader->ReadString();
		SizeT num = reader->ReadInt();
		this->skinLists[this->skinListIndex].name = skinListName;
		this->skinLists[this->skinListIndex].skinNames.Resize(num);
		this->skinLists[this->skinListIndex].skinNodes.Resize(num);

		// add skins to list
		IndexT i;
		for (i = 0; i < num; i++)
			this->skinLists[this->skinListIndex].skinNames[i] = reader->ReadString();
    }
    else
    {
        retval = TransformNode::Load(fourcc, tag, reader);
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterNode::Instance::ApplySkin(const Util::StringAtom& skinName)
{
	IndexT skinIndex = this->activeSkinInstances.FindIndex(skinName);

	// avoid adding the same skin again
	if (skinIndex == InvalidIndex)
	{
		CharacterNode* cnode = (CharacterNode*)this->node;
		IndexT idx = cnode->skinNodes[skinName];

		// activate node and add to the active skin dictionary
		CharacterSkinNode::Instance* snode = (CharacterSkinNode::Instance*)this->children[idx];
		snode->active = true;
		this->activeSkinInstances.Add(skinName, snode);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterNode::Instance::RemoveSkin(const Util::StringAtom& skinName)
{
	IndexT skinIndex = this->activeSkinInstances.FindIndex(skinName);
	n_assert(skinIndex != InvalidIndex);

	// deactivate node
	this->activeSkinInstances.ValueAtIndex(skinName, skinIndex)->active = false;
	this->activeSkinInstances.EraseIndex(skinName, skinIndex);
}

} // namespace Characters
