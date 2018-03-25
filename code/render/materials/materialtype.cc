//------------------------------------------------------------------------------
//  materialtype.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialtype.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
namespace Materials
{

//------------------------------------------------------------------------------
/**
*/
MaterialType::MaterialType() :
	currentAllocator(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
MaterialType::~MaterialType()
{
}

//------------------------------------------------------------------------------
/**
*/
Ids::Id32
MaterialType::CreateMaterial()
{
	Ids::Id32 mat = this->materialAllocator.AllocObject();
	Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
	Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matConstants = this->materialAllocator.Get<1>(mat);
	Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matTextures = this->materialAllocator.Get<2>(mat);
	SizeT i;
	for (i = 0; i < batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = batches[i];
		CoreGraphics::ShaderId shid;
		shid.allocId = programs[batch].shaderId;
		CoreGraphics::ShaderStateId state = CoreGraphics::ShaderCreateState(shid, { NEBULAT_BATCH_GROUP }, false);
		indices.Append(this->states[batch].Size());

		matConstants.Append(Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>());
		Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>& matConstantDict = matConstants.Back();

		matTextures.Append(Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>());
		Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>& matTextureDict = matTextures.Back();
		SizeT j;
		for (j = 0; j < this->constants.Size(); j++)
		{
			CoreGraphics::ShaderConstantId cid = CoreGraphics::ShaderStateGetConstant(state, this->constants.KeyAtIndex(j));
			matConstantDict.Add(this->constants.KeyAtIndex(j), cid);
			CoreGraphics::ShaderConstantSet(cid, state, this->constants.ValueAtIndex(j).default);
		}
		for (j = 0; j < this->textures.Size(); j++)
		{
			CoreGraphics::ShaderConstantId cid = CoreGraphics::ShaderStateGetConstant(state, this->textures.KeyAtIndex(j));
			matTextureDict.Add(this->textures.KeyAtIndex(j), cid);
			CoreGraphics::ShaderResourceSetTexture(cid, state, this->textures.ValueAtIndex(j).default);
		}
		this->states[batch].Append(state);
	}
	return mat;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::DestroyMaterial(Ids::Id32 mat)
{
	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			CoreGraphics::ShaderDestroyState(stateIds[indices[j]]);
		}
	}
	this->materialAllocator.DeallocObject(mat);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetupMaterial(Ids::Id32 mat, MaterialSetup setup)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matConstants = this->materialAllocator.Get<1>(mat);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matTextures = this->materialAllocator.Get<2>(mat);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			const CoreGraphics::ShaderStateId state = stateIds[indices[j]];
			SizeT k;
			for (k = 0; k < setup.constants.Size(); k++)
			{
				CoreGraphics::ShaderConstantSet(matConstants[j][setup.constantNames[k]], state, setup.constants[k].default);
			}
			for (k = 0; k < setup.textures.Size(); k++)
			{
				CoreGraphics::ShaderResourceSetTexture(matConstants[j][setup.textureNames[k]], state, setup.textures[k].default);
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::MaterialSetConstant(Ids::Id32 mat, Util::StringAtom name, const Util::Variant& constant)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matConstants = this->materialAllocator.Get<1>(mat);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			const CoreGraphics::ShaderStateId state = stateIds[indices[j]];
			CoreGraphics::ShaderConstantSet(matConstants[i][name], state, constant);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::MaterialSetTexture(Ids::Id32 mat, Util::StringAtom name, const CoreGraphics::TextureId tex)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matTextures = this->materialAllocator.Get<2>(mat);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			const CoreGraphics::ShaderStateId state = stateIds[indices[j]];
			CoreGraphics::ShaderResourceSetTexture(matTextures[i][name], state, tex);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::BeginBatch(CoreGraphics::BatchGroup::Code batch)
{
	n_assert(this->currentAllocator == nullptr);
	this->currentAllocator = &states[batch];
	CoreGraphics::ShaderProgramBind(this->programs[batch]);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::EndBatch()
{
	n_assert(this->currentAllocator != nullptr);
	this->currentAllocator = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::ApplyMaterial(Ids::Id32 mat)
{
	n_assert(this->currentAllocator != nullptr);
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
	CoreGraphics::ShaderStateApply((*this->currentAllocator)[indices[mat]]);
}
} // namespace Materials
