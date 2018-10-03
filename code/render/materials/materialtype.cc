//------------------------------------------------------------------------------
//  materialtype.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialtype.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
#include "coregraphics/resourcetable.h"
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
MaterialInstanceId
MaterialType::CreateInstance()
{
	Ids::Id32 mat = this->materialAllocator.AllocObject();
	Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat);
	Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ConstantBinding>>& matConstants = this->materialAllocator.Get<1>(mat);
	Util::Array<Util::HashTable<Util::StringAtom, IndexT>>& matTextures = this->materialAllocator.Get<2>(mat);
	SizeT i;
	for (i = 0; i < batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = batches[i];
		CoreGraphics::ShaderId shid;
		shid.allocId = programs[batch].shaderId;
		CoreGraphics::ResourceTableId table = CoreGraphics::ShaderCreateResourceTable(shid, NEBULAT_BATCH_GROUP);

		matConstants.Append(Util::HashTable<Util::StringAtom, CoreGraphics::ConstantBinding>());
		Util::HashTable<Util::StringAtom, CoreGraphics::ConstantBinding>& matConstantDict = matConstants.Back();

		matTextures.Append(Util::HashTable<Util::StringAtom, IndexT>());
		Util::HashTable<Util::StringAtom, IndexT>& matTextureDict = matTextures.Back();
		SizeT j;
		for (j = 0; j < this->constants.Size(); j++)
		{
			if (!this->constants.ValueAtIndex(j).system)
			{
				CoreGraphics::ConstantBinding cid = CoreGraphics::ShaderGetConstantBinding(shid, this->constants.KeyAtIndex(j));
				matConstantDict.Add(this->constants.KeyAtIndex(j), cid);
				if (cid.byteSize != -1)
				{

					//CoreGraphics::ShaderConstantSet(cid, state, this->constants.ValueAtIndex(j).defaultValue);
				}
			}
		}
		for (j = 0; j < this->textures.Size(); j++)
		{
			if (!this->textures.ValueAtIndex(j).system)
			{
				IndexT cid = CoreGraphics::ShaderGetResourceSlot(shid, this->textures.KeyAtIndex(j));
				matTextureDict.Add(this->textures.KeyAtIndex(j), cid);
				if (cid != InvalidIndex)
				{
					//CoreGraphics::ShaderResourceSetTexture(cid, state, this->textures.ValueAtIndex(j).defaultValue);
				}
			}
		}
		Util::Array<CoreGraphics::ResourceTableId>& tables = this->tables.AddUnique(batch);
		CoreGraphics::ResourceTableCommitChanges(table);
		tables.Append(table);

		indices.Append(this->tables[batch].Size()-1);
	}
	return mat;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::DestroyInstance(MaterialInstanceId mat)
{
	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ResourceTableId>& tableIds = this->tables[batch];
		const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			CoreGraphics::DestroyResourceTable(tableIds[indices[j]]);
		}
	}
	this->materialAllocator.DeallocObject(mat.id);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetConstant(MaterialInstanceId mat, Util::StringAtom name, const Util::Variant& constant)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
	const Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ConstantBinding>>& matConstants = this->materialAllocator.Get<1>(mat.id);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ResourceTableId>& tableIds = this->tables[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			if (matConstants[i][name].byteSize != -1)
			{
				const CoreGraphics::ResourceTableId table = tableIds[indices[j]];
				//CoreGraphics::ShaderConstantSet(matConstants[i][name], state, constant);
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetTexture(MaterialInstanceId mat, Util::StringAtom name, const CoreGraphics::TextureId tex)
{
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
	const Util::Array<Util::HashTable<Util::StringAtom, IndexT>>& matTextures = this->materialAllocator.Get<2>(mat.id);

	SizeT i;
	for (i = 0; i < this->batches.Size(); i++)
	{
		const CoreGraphics::BatchGroup::Code batch = this->batches[i];
		const Util::Array<CoreGraphics::ResourceTableId>& tableIds = this->tables[batch];
		SizeT j;
		for (j = 0; j < indices.Size(); j++)
		{
			if (matTextures[i][name] != InvalidIndex)
			{
				const CoreGraphics::ResourceTableId state = tableIds[indices[j]];
				//CoreGraphics::ShaderResourceSetTexture(matTextures[i][name], state, tex);
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
MaterialType::BeginBatch(CoreGraphics::BatchGroup::Code batch)
{
	n_assert(this->currentAllocator == nullptr);
	if (tables.Contains(batch))
	{
		this->currentAllocator = &tables[batch];
		CoreGraphics::SetShaderProgram(this->programs[batch]);
		return true;
	}
	return false;
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
MaterialType::ApplyInstance(const MaterialInstanceId& mat)
{
	n_assert(this->currentAllocator != nullptr);
	const Util::Array<Ids::Id32>& indices = this->materialAllocator.Get<0>(mat.id);
	CoreGraphics::SetResourceTable((*this->currentAllocator)[indices[mat.id]], NEBULAT_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}
} // namespace Materials
