//------------------------------------------------------------------------------
//  charactermaterialskinnode.cc
//  (C) 2011-2018 Individual contributors, see AUTHORS file
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
	this->cboSkin = CoreGraphics::ShaderCreateConstantBuffer(this->sharedShader, "JointBlock");
	this->cboSkinIndex = CoreGraphics::ShaderGetResourceSlot(this->sharedShader, "JointBlock");
	CoreGraphics::ResourceTableSetConstantBuffer(this->resourceTable, { this->cboSkin, this->cboSkinIndex, 0, true, false, -1, 0 });
	CoreGraphics::ResourceTableCommitChanges(this->resourceTable);
	this->skinningPaletteVar = CoreGraphics::ShaderGetConstantBinding(this->sharedShader, "JointPalette");
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
	const CharacterNode::Instance* cparent = static_cast<const CharacterNode::Instance*>(this->parent);

	// if parent doesn't have joints, don't continue
	if (cparent->joints != nullptr)
	{
		CharacterSkinNode* sparent = static_cast<CharacterSkinNode*>(this->node);
		const Util::Array<IndexT>& usedIndices = sparent->skinFragments[0].jointPalette;
		Util::FixedArray<Math::matrix44> usedMatrices(usedIndices.Size());

		// copy active matrix palette, or set identity
		IndexT i;
		for (i = 0; i < usedIndices.Size(); i++)
		{
			usedMatrices[i] = (*cparent->joints)[usedIndices[i]];
		}

		// update skinning palette
		CoreGraphics::ConstantBufferUpdate(this->cboSkin, this->cboSkinAlloc, usedMatrices.Begin(), sizeof(Math::matrix44) * usedMatrices.Size(), this->skinningPaletteVar);
	}

	// apply original state
	PrimitiveNode::Instance::Update();
}

} // namespace Characters
