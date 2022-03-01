//------------------------------------------------------------------------------
//  charactermaterialskinnode.cc
//  (C) 2011-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "characterskinnode.h"
#include "coregraphics/shaderserver.h"

namespace Models
{

using namespace Util;
using namespace Models;
using namespace IO;
using namespace CoreGraphics;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
CharacterSkinNode::CharacterSkinNode()
{
    this->skinnedShaderFeatureBits = ShaderServer::Instance()->FeatureStringToMask("Skinned");
    this->type = CharacterSkinNodeType;
    this->bits = HasTransformBit | HasStateBit;
}

//------------------------------------------------------------------------------
/**
*/
CharacterSkinNode::~CharacterSkinNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
CharacterSkinNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
{
    bool retval = true;
    if (FourCC('NSKF') == fourcc)
    {
        // NumSkinFragments
        this->ReserveFragments(reader->ReadInt());
    }
    else if (FourCC('SFRG') == fourcc)
    {
        // SkinFragment
        this->primitiveGroupIndex = reader->ReadInt();
        Array<IndexT> jointPalette;
        SizeT numJoints = reader->ReadInt();
        jointPalette.Reserve(numJoints);
        IndexT i;
        for (i = 0; i < numJoints; i++)
        {
            jointPalette.Append(reader->ReadInt());
        }
        this->AddFragment(this->primitiveGroupIndex, jointPalette);
    }
    else
    {
        retval = PrimitiveNode::Load(fourcc, tag, reader, immediate);
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterSkinNode::OnFinishedLoading()
{
    PrimitiveNode::OnFinishedLoading();
    CoreGraphics::ShaderId shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:objects_shared.fxb"_atm);
    CoreGraphics::BufferId cbo = CoreGraphics::GetGraphicsConstantBuffer();
    IndexT index = CoreGraphics::ShaderGetResourceSlot(shader, "JointBlock");
    CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { cbo, index, 0, false, true, (SizeT)(sizeof(Math::mat4) * this->skinFragments[0].jointPalette.Size()), 0 });
    CoreGraphics::ResourceTableCommitChanges(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
std::function<const CoreGraphics::PrimitiveGroup()>
CharacterSkinNode::GetPrimitiveGroupFunction()
{
    return [this]()
    {
        return this->primGroup;
    };
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterSkinNode::ReserveFragments(SizeT numFragments)
{
    this->skinFragments.Clear();
    this->skinFragments.Reserve(numFragments);
}

//------------------------------------------------------------------------------
/**
*/
void
CharacterSkinNode::AddFragment(IndexT primGroupIndex, const Util::Array<IndexT>& jointPalette)
{
    Fragment fragment;
    this->skinFragments.Append(fragment);
    this->skinFragments.Back().primGroupIndex = primGroupIndex;
    this->skinFragments.Back().jointPalette = jointPalette;
}

} // namespace Characters
