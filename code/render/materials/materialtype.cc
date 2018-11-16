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
MaterialType::MaterialType()
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
void 
MaterialType::Setup()
{
	// setup binding in each program (should be identical)
	auto it = this->batchToIndexMap.Begin();
	while (it != this->batchToIndexMap.End())
	{
		const CoreGraphics::ShaderId shd = this->programs[*it.val].As<CoreGraphics::ShaderId>();
		IndexT i;
		for (i = 0; i < this->textures.Size(); i++)
		{
			MaterialTexture tex = this->textures.ValueAtIndex(i);
			tex.slot = CoreGraphics::ShaderGetResourceSlot(shd, tex.name.AsCharPtr());
			if (tex.slot != InvalidIndex)
			{
				this->texturesByBatch[*it.val].Add(tex.name, tex);
			}
			else
			{
				this->texturesByBatch[*it.val].Add(tex.name, { tex.name, CoreGraphics::TextureId::Invalid(), CoreGraphics::InvalidTextureType, false, InvalidIndex });
			}
		}

		for (i = 0; i < this->constants.Size(); i++)
		{
			MaterialConstant constant = this->constants.ValueAtIndex(i);
			constant.slot = CoreGraphics::ShaderGetConstantSlot(shd, constant.name);
			
			// only bind if there is a binding
			if (constant.slot != -1)
			{
				constant.offset = CoreGraphics::ShaderGetConstantBinding(shd, constant.name.AsCharPtr());
				constant.group = CoreGraphics::ShaderGetConstantGroup(shd, constant.name);
				this->constantsByBatch[*it.val].Add(constant.name, constant);
			}
			else
			{
				this->constantsByBatch[*it.val].Add(constant.name, { constant.name, constant.defaultValue, nullptr, nullptr, constant.defaultValue.GetType(), false, UINT_MAX, InvalidIndex, InvalidIndex });
			}
		}

		it++;
	}
}

//------------------------------------------------------------------------------
/**
*/
SurfaceId
MaterialType::CreateSurface()
{
	Ids::Id32 sur = this->surfaceAllocator.AllocObject();

	// resize all arrays
	this->surfaceAllocator.Get<SurfaceTable>(sur).Resize(this->batchToIndexMap.Size()); // surface tables
	this->surfaceAllocator.Get<InstanceTable>(sur).Resize(this->batchToIndexMap.Size()); // instance tables
	this->surfaceAllocator.Get<SurfaceBuffers>(sur).Resize(this->batchToIndexMap.Size()); // surface buffers
	this->surfaceAllocator.Get<InstanceBuffers>(sur).Resize(this->batchToIndexMap.Size()); // instance buffers
	this->surfaceAllocator.Get<Textures>(sur).Resize(this->batchToIndexMap.Size()); // textures
	this->surfaceAllocator.Get<Constants>(sur).Resize(this->batchToIndexMap.Size()); // constants
	
	// go through all batches
	auto batchIt = this->batchToIndexMap.Begin();
	while (batchIt != this->batchToIndexMap.End())
	{
		const CoreGraphics::BatchGroup::Code code = *batchIt.key;
		const CoreGraphics::ShaderProgramId prog = this->programs[*batchIt.val];

		// create temporary shader id (this is safe)
		const CoreGraphics::ShaderId shd = prog.As<const CoreGraphics::ShaderId>();

		// create resource tables
		CoreGraphics::ResourceTableId surfaceTable = CoreGraphics::ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP);
		this->surfaceAllocator.Get<SurfaceTable>(sur)[*batchIt.val] = surfaceTable;
		CoreGraphics::ResourceTableId instanceTable = CoreGraphics::ShaderCreateResourceTable(shd, NEBULA_INSTANCE_GROUP);
		this->surfaceAllocator.Get<InstanceTable>(sur)[*batchIt.val] = instanceTable;
		
		// get constant buffer count
		SizeT numBuffers = CoreGraphics::ShaderGetConstantBufferCount(shd);

		// get arrays to pre-allocated buffers
		Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId>>& surfaceBuffers = this->surfaceAllocator.Get<SurfaceBuffers>(sur)[*batchIt.val];
		Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId>>& instanceBuffers = this->surfaceAllocator.Get<InstanceBuffers>(sur)[*batchIt.val];

		// create instance of constant buffers
		IndexT j;
		for (j = 0; j < numBuffers; j++)
		{
			IndexT slot = CoreGraphics::ShaderGetConstantBufferResourceSlot(shd, j);
			IndexT group = CoreGraphics::ShaderGetConstantBufferResourceGroup(shd, j);
			if (group == NEBULA_BATCH_GROUP && surfaceTable != CoreGraphics::ResourceTableId::Invalid())
			{
				CoreGraphics::ConstantBufferId buf = CoreGraphics::ShaderCreateConstantBuffer(shd, j);
				if (buf != CoreGraphics::ConstantBufferId::Invalid())
				{
					CoreGraphics::ResourceTableSetConstantBuffer(surfaceTable, { buf, slot, 0, false, false, -1, 0 });

					// add to surface
					surfaceBuffers.Append(std::make_tuple(slot, buf));
				}
			}			
			else if (group == NEBULA_INSTANCE_GROUP && instanceTable != CoreGraphics::ResourceTableId::Invalid())
			{
				CoreGraphics::ConstantBufferId buf = CoreGraphics::ShaderCreateConstantBuffer(shd, j);
				if (buf != CoreGraphics::ConstantBufferId::Invalid())
				{
					CoreGraphics::ResourceTableSetConstantBuffer(instanceTable, { buf, slot, 0, true, false, -1, 0 });

					// add to surface
					instanceBuffers.Append(std::make_tuple(slot, buf));
				}
			}
		}

		// setup textures
		const Util::Dictionary<Util::StringAtom, MaterialTexture>& textures = this->texturesByBatch[*batchIt.val];
		if (surfaceTable != CoreGraphics::ResourceTableId::Invalid()) 
			for (j = 0; j < textures.Size(); j++)
			{
				const MaterialTexture& tex = textures.ValueAtIndex(j);
				SurfaceTexture surTex;
				surTex.slot = tex.slot;
				surTex.defaultValue = tex.defaultValue;
				if (tex.slot != InvalidIndex)
					CoreGraphics::ResourceTableSetTexture(surfaceTable, { tex.defaultValue, tex.slot, 0, CoreGraphics::SamplerId::Invalid(), false });

				if (batchIt == this->batchToIndexMap.Begin())
					this->surfaceAllocator.Get<TextureMap>(sur).Add(tex.name, this->surfaceAllocator.Get<Textures>(sur)[*batchIt.val].Size());

				this->surfaceAllocator.Get<Textures>(sur)[*batchIt.val].Append(surTex);
			}

		// update tables
		if (surfaceTable != CoreGraphics::ResourceTableId::Invalid())
			CoreGraphics::ResourceTableCommitChanges(surfaceTable);

		if (instanceTable != CoreGraphics::ResourceTableId::Invalid())
			CoreGraphics::ResourceTableCommitChanges(instanceTable);

		const Util::Dictionary<Util::StringAtom, MaterialConstant>& constants = this->constantsByBatch[*batchIt.val];
		for (j = 0; j < constants.Size(); j++)
		{
			const MaterialConstant& constant = constants.ValueAtIndex(j);
			SurfaceConstant surConst;
			surConst.defaultValue = constant.defaultValue;
			surConst.binding = constant.offset;
			surConst.bufferIndex = InvalidIndex;
			surConst.instanceConstant = false;
			surConst.buffer = CoreGraphics::ConstantBufferId::Invalid();
			if (constant.group == NEBULA_BATCH_GROUP)
			{
				surConst.instanceConstant = false;
				// go through surface-level buffers to find slot which matches
				IndexT k;
				for (k = 0; k < surfaceBuffers.Size(); k++)
				{
					if (std::get<0>(surfaceBuffers[k]) == constant.slot)
					{
						surConst.bufferIndex = k;
						surConst.buffer = std::get<1>(surfaceBuffers[k]);
						break;
					}
				}
			}
			else if (constant.group == NEBULA_INSTANCE_GROUP)
			{
				surConst.instanceConstant = true;
				// go through instance-level buffers to find slot which matches
				IndexT k;
				for (k = 0; k < instanceBuffers.Size(); k++)
				{
					if (std::get<0>(instanceBuffers[k]) == constant.slot)
					{
						surConst.bufferIndex = k;
						surConst.buffer = std::get<1>(instanceBuffers[k]);
						break;
					}
				}
			}
#if NEBULA_DEBUG
			else
			{
				n_warning("Material constant %s does not belong to a constant buffer bound to either the BATCH or INSTANCE group\n", constant.name.AsCharPtr());
			}
#endif
			if (batchIt == this->batchToIndexMap.Begin())
				this->surfaceAllocator.Get<ConstantMap>(sur).Add(constant.name, this->surfaceAllocator.Get<Constants>(sur)[*batchIt.val].Size());

			this->surfaceAllocator.Get<Constants>(sur)[*batchIt.val].Append(surConst);
			
		}

		batchIt++;
	}

	return sur;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::DestroySurface(SurfaceId sur)
{
	this->surfaceAllocator.DeallocObject(sur.id);
}

//------------------------------------------------------------------------------
/**
*/
SurfaceInstanceId 
MaterialType::CreateSurfaceInstance(const SurfaceId id)
{
	Ids::Id32 inst = this->surfaceInstanceAllocator.AllocObject();

	this->surfaceInstanceAllocator.Get<SurfaceInstanceConstants>(inst).Resize(this->batchToIndexMap.Size());
	this->surfaceInstanceAllocator.Get<ConstantBufferOffsets>(inst).Resize(this->batchToIndexMap.Size());
	this->surfaceInstanceAllocator.Get<ConstantBufferInstance>(inst).Resize(this->batchToIndexMap.Size());

	auto batchIt = this->batchToIndexMap.Begin();
	while (batchIt != this->batchToIndexMap.End())
	{
		// get surface level stuff
		const Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId>>& instanceBuffers = this->surfaceAllocator.Get<InstanceBuffers>(id.id)[*batchIt.val];
		const Util::Array<SurfaceConstant>& constants = this->surfaceAllocator.Get<Constants>(id.id)[*batchIt.val];
		const CoreGraphics::ResourceTableId instanceTable = this->surfaceAllocator.Get<InstanceTable>(id.id)[*batchIt.val];

		// get instance level stuff
		Util::FixedArray<SurfaceInstanceConstant>& surfaceInstanceConstants = this->surfaceInstanceAllocator.Get<SurfaceInstanceConstants>(inst)[*batchIt.val];
		Util::FixedArray<uint>& bufferOffsets = this->surfaceInstanceAllocator.Get<ConstantBufferOffsets>(inst)[*batchIt.val];
		Util::FixedArray<uint>& bufferInstances = this->surfaceInstanceAllocator.Get<ConstantBufferInstance>(inst)[*batchIt.val];

		// resize 
		surfaceInstanceConstants.Resize(constants.Size());
		bufferOffsets.Resize(instanceBuffers.Size());
		bufferInstances.Resize(instanceBuffers.Size());

		bool rebind = false;
		IndexT i;
		for (i = 0; i < instanceBuffers.Size(); i++)
		{
			if (CoreGraphics::ConstantBufferAllocateInstance(std::get<1>(instanceBuffers[i]), bufferOffsets[i], bufferInstances[i]))
			{
				CoreGraphics::ResourceTableSetConstantBuffer(instanceTable, { std::get<1>(instanceBuffers[i]), std::get<0>(instanceBuffers[i]), 0, true, false, -1, 0 });
				rebind = true;
			}
		}

		// make sure to commit changes if any were applied
		if (rebind)
			CoreGraphics::ResourceTableCommitChanges(instanceTable);

		// setup instance constants
		for (i = 0; i < constants.Size(); i++)
		{
			const SurfaceConstant& constant = constants[i];
			SurfaceInstanceConstant& instanceConstant = this->surfaceInstanceAllocator.Get<SurfaceInstanceConstants>(inst)[*batchIt.val][i];
			if (constant.instanceConstant)
			{
				CoreGraphics::ConstantBinding newBinding = constant.binding;
				newBinding.offset += bufferOffsets[constant.bufferIndex];
				instanceConstant.binding = newBinding;
			}
			else
			{
				instanceConstant.binding = { UINT_MAX };
			}
		}

		batchIt++;
	}
	
	// create id
	SurfaceInstanceId ret;
	ret.instance = inst;
	ret.surface = id.id;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialType::DestroySurfaceInstance(const SurfaceInstanceId id)
{
	this->surfaceInstanceAllocator.DeallocObject(id.instance);
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
MaterialType::GetSurfaceConstantIndex(const SurfaceId sur, const Util::StringAtom& name)
{
	IndexT idx = this->surfaceAllocator.Get<ConstantMap>(sur.id).FindIndex(name);
	if (idx != InvalidIndex)	return this->surfaceAllocator.Get<ConstantMap>(sur.id).ValueAtIndex(idx);
	else						return idx;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
MaterialType::GetSurfaceTextureIndex(const SurfaceId sur, const Util::StringAtom& name)
{
	IndexT idx = this->surfaceAllocator.Get<TextureMap>(sur.id).FindIndex(name);
	if (idx != InvalidIndex)	return this->surfaceAllocator.Get<TextureMap>(sur.id).ValueAtIndex(idx);
	else						return idx;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
MaterialType::GetSurfaceConstantInstanceIndex(const SurfaceInstanceId sur, const Util::StringAtom& name)
{
	IndexT idx = this->surfaceAllocator.Get<ConstantMap>(sur.surface).FindIndex(name);
	if (idx != InvalidIndex)	return this->surfaceAllocator.Get<ConstantMap>(sur.surface).ValueAtIndex(idx);
	else						return idx;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Variant 
MaterialType::GetSurfaceConstantDefault(const SurfaceId sur, IndexT idx)
{
	return (*this->surfaceAllocator.Get<Constants>(sur.id).Begin())[idx].defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureId
MaterialType::GetSurfaceTextureDefault(const SurfaceId sur, IndexT idx)
{
	return (*this->surfaceAllocator.Get<Textures>(sur.id).Begin())[idx].defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetSurfaceConstant(const SurfaceId sur, IndexT name, const Util::Variant& value)
{
	auto it = this->batchToIndexMap.Begin();
	while (it != this->batchToIndexMap.End())
	{
		const SurfaceConstant& constant = this->surfaceAllocator.Get<Constants>(sur.id)[*it.val][name];
		if (constant.buffer != CoreGraphics::ConstantBufferId::Invalid() && constant.binding.offset != UINT_MAX)
		{
			n_assert(!constant.instanceConstant);
			CoreGraphics::ConstantBufferUpdate(constant.buffer, value, constant.binding);
		}
		it++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::SetSurfaceTexture(const SurfaceId sur, IndexT name, const CoreGraphics::TextureId tex)
{
	auto it = this->batchToIndexMap.Begin();
	while (it != this->batchToIndexMap.End())
	{
		const SurfaceTexture& surTex = this->surfaceAllocator.Get<Textures>(sur.id)[*it.val][name];
		if (surTex.slot != InvalidIndex)
			CoreGraphics::ResourceTableSetTexture(this->surfaceAllocator.Get<SurfaceTable>(sur.id)[*it.val], { tex, surTex.slot, 0, CoreGraphics::SamplerId::Invalid(), false });
		it++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialType::SetSurfaceInstanceConstant(const SurfaceInstanceId sur, const IndexT idx, const Util::Variant& value)
{
	auto it = this->batchToIndexMap.Begin();
	while (it != this->batchToIndexMap.End())
	{
		const SurfaceInstanceConstant& constant = this->surfaceInstanceAllocator.Get<0>(sur.instance)[*it.val][idx];
		if (constant.buffer != CoreGraphics::ConstantBufferId::Invalid() && constant.binding.offset != UINT_MAX)
			CoreGraphics::ConstantBufferUpdate(constant.buffer, value, constant.binding);
		it++;
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
MaterialType::BeginBatch(CoreGraphics::BatchGroup::Code batch)
{
	IndexT idx = this->batchToIndexMap[batch];
	if (idx != InvalidIndex)
	{
		CoreGraphics::SetShaderProgram(this->programs[idx]);
		this->currentBatch = idx;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
MaterialType::BeginSurface(const SurfaceId id)
{
	n_assert(this->currentBatch != CoreGraphics::BatchGroup::InvalidBatchGroup);
	if (this->currentBatch != InvalidIndex)
	{
		this->currentSurfaceBatchIndex = this->currentBatch;
		CoreGraphics::SetResourceTable(this->surfaceAllocator.Get<SurfaceTable>(id.id)[this->currentBatch], NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::ApplyInstance(const SurfaceInstanceId id)
{
	n_assert(this->currentBatch != CoreGraphics::BatchGroup::InvalidBatchGroup);
	n_assert(this->currentSurfaceBatchIndex != InvalidIndex);
	CoreGraphics::SetResourceTable(this->surfaceAllocator.Get<InstanceTable>(id.surface)[this->currentSurfaceBatchIndex], NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, this->surfaceInstanceAllocator.Get<ConstantBufferOffsets>(id.instance)[this->currentSurfaceBatchIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialType::EndSurface()
{
	this->currentSurfaceBatchIndex = -1;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::EndBatch()
{
	this->currentBatch = CoreGraphics::BatchGroup::InvalidBatchGroup;
}


} // namespace Materials
