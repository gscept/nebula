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
    //n_assert(this->character.isvalid());
}

//------------------------------------------------------------------------------
/**
    Recursively create node instances and attach them to the provided
    model instance. The character node will not initially create node
    instances for the skins, but will only create skin node instances
    when they are actually set to visible. This eliminates a lot of
    dead weight for invisible skins!
Ptr<ModelNodeInstance>
CharacterNode::RecurseCreateNodeInstanceHierarchy(const Ptr<ModelInstance>& modelInst, const Ptr<ModelNodeInstance>& parentNodeInst)
{
    // create a ModelNodeInstance of myself
    Ptr<ModelNodeInstance> myNodeInst = this->CreateNodeInstance();
    myNodeInst->Setup(modelInst, this, parentNodeInst);

    // DO NOT recurse into children (the child nodes represent the
    // skin model nodes)
    return myNodeInst;
}
*/

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
	// setup the managed resource
	//this->managedAnimResource = Resources::CreateResource(this->animResId, this->tag, [this](Resources::ResourceId) { this->OnResourcesLoaded(); }, nullptr, false);

	//	ResourceManager::Instance()->CreateManagedResource(AnimResource::RTTI, this->animResId, 0, sync).downcast<ManagedAnimResource>();
	//n_assert(this->managedAnimResource != Resources::ResourceId::Invalid());

	// setup the character's skin library from our children
	// (every child node represents one character skin)
	// FIXME: handle skin category!
	/*
	CharacterSkinLibrary& skinLib = this->character->SkinLibrary();
	StringAtom category("UnknownCategory");
	SizeT numSkins = this->children.Size();
	IndexT skinIndex;
	skinLib.ReserveSkins(numSkins);
	for (skinIndex = 0; skinIndex < numSkins; skinIndex++)
	{
	CharacterSkin skin(this->children[skinIndex], category, this->children[skinIndex]->GetName());
	this->character->SkinLibrary().AddSkin(skin);
	}

	this->sharedShader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);
	this->cbo = CoreGraphics::ShaderCreateConstantBuffer(this->sharedShader, "JointBlock");
	this->cboIndex = CoreGraphics::ShaderGetResourceSlot(this->sharedShader, "JointBlock");

	/*
	if (this->variationResId.IsValid())
	{
	// setup the managed resource for variations
	this->managedVariationResource = ResourceManager::Instance()->CreateManagedResource(AnimResource::RTTI, this->variationResId, 0, sync).downcast<ManagedAnimResource>();
	n_assert(this->managedVariationResource.isvalid());
	}
	*/
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
    if (FourCC('ANIM') == fourcc)
    {
        // Animation
        this->animResId = reader->ReadString();
		this->tag = tag;
    }
    else if (FourCC('NJNT') == fourcc)
    {
        // NumJoints
        SizeT numJoints = reader->ReadInt();
        //this->character->Skeleton().Setup(numJoints);
    }
    else if (FourCC('JONT') == fourcc)
    {
        // Joint
        IndexT jointIndex       = reader->ReadInt();
        IndexT parentJointIndex = reader->ReadInt();
        vector poseTranslation  = reader->ReadFloat4();
        quaternion poseRotation = quaternion(reader->ReadFloat4());
        vector poseScale        = reader->ReadFloat4();
        StringAtom jointName    = reader->ReadString();

        // FIXME: Maya likes to return quaternions with de-normalized numbers in it,
        // this should better be fixed by the asset pipeline!
        // poseRotation.undenormalize();

        //this->character->Skeleton().SetupJoint(jointIndex, parentJointIndex, poseTranslation, poseRotation, poseScale, jointName);
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
        // NumSkinLists
        //this->character->SkinLibrary().ReserveSkinLists(reader->ReadInt());
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


} // namespace Characters
