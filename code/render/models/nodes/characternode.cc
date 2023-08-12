//------------------------------------------------------------------------------
//  characternode.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "characternode.h"

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
    this->bits = HasTransformBit;
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
CharacterNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
{
    bool retval = true;
    this->tag = tag;

    if (FourCC('ANIM') == fourcc)
    {
        // Animation
        this->animResId = reader->ReadString();
    }
    else if (FourCC('ANID') == fourcc)
    {
        this->animIndex = reader->ReadInt();
    }
    else if (FourCC('SKEL') == fourcc)
    {
        this->skeletonResId = reader->ReadString();
    }
    else if (FourCC('SKID') == fourcc)
    {
        this->skeletonIndex = reader->ReadInt();
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
    else
    {
        retval = TransformNode::Load(fourcc, tag, reader, immediate);
    }
    return retval;
}

} // namespace Characters
