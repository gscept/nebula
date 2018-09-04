//------------------------------------------------------------------------------
//  materialtype.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialtype.h"
#include "coregraphics/graphicsdevice.h"
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
MaterialId
MaterialType::CreateInstance()
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

		matConstants.Append(Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>());
		Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>& matConstantDict = matConstants.Back();

		matTextures.Append(Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>());
		Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>& matTextureDict = matTextures.Back();
		SizeT j;
		for (j = 0; j < this->constants.Size(); j++)
		{
			if (!this->constants.ValueAtIndex(j).system)
			{
				CoreGraphics::ShaderConstantId cid = CoreGraphics::ShaderStateGetConstant(state, this->constants.KeyAtIndex(j));
				matConstantDict.Add(this->constants.KeyAtIndex(j), cid);
				if (cid != CoreGraphics::ShaderConstantId::Invalid())
				{
					CoreGraphics::ShaderConstantSet(cid, state, this->constants.ValueAtIndex(j).default);
				}
			}
		}
		for (j = 0; j < this->textures.Size(); j++)
		{
			if (!this->textures.ValueAtIndex(j).system)
			{
				CoreGraphics::ShaderConstantId cid = CoreGraphics::ShaderStateGetConstant(state, this->textures.KeyAtIndex(j));
				matTextureDict.Add(this->textures.KeyAtIndex(j), cid);
				if (cid != CoreGraphics::ShaderConstantId::Invalid())
				{
					CoreGraphics::ShaderResourceSetTexture(cid, state, this->textures.ValueAtIndex(j).default);
				}
			}
		}
		Util::Array<CoreGraphics::ShaderStateId>& shaderStates = this->states.AddUnique(batch);
		shaderStates.Append(state);

		indices.Append(this->states[batch].Size()-1);
	}
	return mat;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::DestroyInstance(MaterialId mat)
{
	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			CoreGraphics::ShaderDestroyState(stateIds[indices[j]]);
		}
	}
	this->materialAllocator.DeallocObject(mat.id);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetConstant(MaterialId mat, Util::StringAtom name, const Util::Variant& constant)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matConstants = this->materialAllocator.Get<1>(mat.id);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			if (matConstants[i][name] != CoreGraphics::ShaderConstantId::Invalid())
			{
				const CoreGraphics::ShaderStateId state = stateIds[indices[j]];
				CoreGraphics::ShaderConstantSet(matConstants[i][name], state, constant);
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetTexture(MaterialId mat, Util::StringAtom name, const CoreGraphics::TextureId tex)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>& matTextures = this->materialAllocator.Get<2>(mat.id);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ShaderStateId>& stateIds = this->states[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			if (matTextures[i][name] != CoreGraphics::ShaderConstantId::Invalid())
			{
				const CoreGraphics::ShaderStateId state = stateIds[indices[j]];
				CoreGraphics::ShaderResourceSetTexture(matTextures[i][name], state, tex);
			}
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
	if (states.Contains(batch))
	{
		this->currentAllocator = &states[batch];
		CoreGraphics::SetShaderProgram(this->programs[batch]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::EndBatch()
{
	this->currentAllocator = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::ApplyInstance(const MaterialId& mat)
{
	n_assert(this->currentAllocator != nullptr);
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
	CoreGraphics::SetShaderState((*this->currentAllocator)[indices[mat.id]]);
}
} // namespace Materials
