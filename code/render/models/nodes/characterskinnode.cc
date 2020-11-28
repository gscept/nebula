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
        IndexT primGroupIndex = reader->ReadInt();
        Array<IndexT> jointPalette;
        SizeT numJoints = reader->ReadInt();
        jointPalette.Reserve(numJoints);
        IndexT i;
        for (i = 0; i < numJoints; i++)
        {
            jointPalette.Append(reader->ReadInt());
        }
        this->AddFragment(primGroupIndex, jointPalette);
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
	CoreGraphics::BufferId cbo = CoreGraphics::GetGraphicsConstantBuffer(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer);
	IndexT index = CoreGraphics::ShaderGetResourceSlot(shader, "JointBlock");
	CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { cbo, index, 0, false, true, (SizeT)(sizeof(Math::mat4) * this->skinFragments[0].jointPalette.Size()), 0 });
	CoreGraphics::ResourceTableCommitChanges(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
void 
CharacterSkinNode::ApplyNodeState()
{
	ShaderStateNode::ApplyNodeState(); // intentionally circumvent PrimitiveNode since we set the skin fragments explicitly
	CoreGraphics::MeshBind(this->res, this->skinFragments[0].primGroupIndex);
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

//------------------------------------------------------------------------------
/**
*/
void
CharacterSkinNode::Instance::Update()
{
	const CharacterNode::Instance* cparent = reinterpret_cast<const CharacterNode::Instance*>(this->parent);
	if (!this->dirty)
		return;

	// apply original state
	PrimitiveNode::Instance::Update();

	// if parent doesn't have joints, don't continue
	CharacterSkinNode* sparent = reinterpret_cast<CharacterSkinNode*>(this->node);
	const Util::Array<IndexT>& usedIndices = sparent->skinFragments[0].jointPalette;
	Util::FixedArray<Math::mat4> usedMatrices(usedIndices.Size());
	if (cparent->joints != nullptr)
	{
		// copy active matrix palette, or set identity
		IndexT i;
		for (i = 0; i < usedIndices.Size(); i++)
		{
			usedMatrices[i] = (*cparent->joints)[usedIndices[i]];
		}
	}
	else
	{
		// copy active matrix palette, or set identity
		IndexT i;
		for (i = 0; i < usedIndices.Size(); i++)
		{
			usedMatrices[i] = Math::mat4();
		}
	}

	// update skinning palette
	uint offset = CoreGraphics::SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer, usedMatrices.Begin(), usedMatrices.Size());
	this->offsets[this->skinningTransformsIndex] = offset;
}

} // namespace Characters
