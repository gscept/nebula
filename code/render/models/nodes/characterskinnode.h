#pragma once
//------------------------------------------------------------------------------
/**
    @class Characters::CharacterMaterialSkinNode
    
	A model node that handles materials for characters
    
    (C) 2011-2020 Individual contributors, see AUTHORS file
*/
#include "primitivenode.h"
#include "characternode.h"
#include "coregraphics/graphicsdevice.h"

//------------------------------------------------------------------------------
namespace Models
{
class CharacterSkinNode : public PrimitiveNode
{
public:
    /// constructor
    CharacterSkinNode();
    /// destructor
    virtual ~CharacterSkinNode();
    
    /// reserve fragments (call before adding fragments)
    void ReserveFragments(SizeT numFragments);
	/// add a fragment (consisting of a mesh group index and a joint palette)
    void AddFragment(IndexT primGroupIndex, const Util::Array<IndexT>& jointPalette);
    /// get number of skin fragments
    SizeT GetNumFragments() const;
    /// get primitive group index of a fragment
    IndexT GetFragmentPrimGroupIndex(IndexT fragmentIndex) const;
    /// get joint palette of a fragment
    const Util::Array<IndexT>& GetFragmentJointPalette(IndexT fragmentIndex) const;

	struct Instance : public PrimitiveNode::Instance
	{
		CoreGraphics::ShaderId skinShader;
		CoreGraphics::ConstantBinding jointPaletteVar;
		Ids::Id32 characterId;
		CoreGraphics::ConstantBufferId cboSkin;
		CoreGraphics::ConstantBinding skinningPaletteVar;

		/// apply skinning palette
		void Update() override;
		/// setup
		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;

	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }
private:
    /// a skin fragment
    class Fragment
    {
    public:
        /// constructor
        Fragment() : primGroupIndex(InvalidIndex) {};

        IndexT primGroupIndex;
        Util::Array<IndexT> jointPalette;
    };

protected:

	/// parse data tag (called by loader code)
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;
	/// called when loading finished
	virtual void OnFinishedLoading();
	/// apply state
	void ApplyNodeState() override;

	CoreGraphics::ConstantBufferId cboSkin;
	IndexT cboSkinIndex;
	CoreGraphics::ConstantBinding skinningPaletteVar;
    CoreGraphics::ShaderFeature::Mask skinnedShaderFeatureBits;
    Util::Array<Fragment> skinFragments;
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
CharacterSkinNode::GetNumFragments() const
{
    return this->skinFragments.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
CharacterSkinNode::GetFragmentPrimGroupIndex(IndexT fragmentIndex) const
{
    return this->skinFragments[fragmentIndex].primGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<IndexT>& 
CharacterSkinNode::GetFragmentJointPalette(IndexT fragmentIndex) const
{
    return this->skinFragments[fragmentIndex].jointPalette;
}

ModelNodeInstanceCreator(CharacterSkinNode)

//------------------------------------------------------------------------------
/**
*/
inline void
CharacterSkinNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	PrimitiveNode::Instance::Setup(node, parent);
	CharacterSkinNode* sparent = static_cast<CharacterSkinNode*>(node);
	const CharacterNode::Instance* cparent = static_cast<const CharacterNode::Instance*>(this->parent);
	this->cboSkin = sparent->cboSkin;
	this->skinningPaletteVar = sparent->skinningPaletteVar;
}

} // namespace Characters
//------------------------------------------------------------------------------
