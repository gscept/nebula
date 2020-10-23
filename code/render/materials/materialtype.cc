//------------------------------------------------------------------------------
//  materialtype.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialtype.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/shader.h"
#include "coregraphics/config.h"
#include "coregraphics/resourcetable.h"
namespace Materials
{

IndexT MaterialType::MaterialTypeUniqueIdCounter = 0;
//------------------------------------------------------------------------------
/**
*/
MaterialType::MaterialType() 
	: currentBatch(CoreGraphics::BatchGroup::InvalidBatchGroup)
	, currentBatchIndex(InvalidIndex)
	, vertexType(-1)
	, isVirtual(false)
	, uniqueId(MaterialTypeUniqueIdCounter++)
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
	Ids::Id32 sur = this->surfaceAllocator.Alloc();

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
		Util::Array<Util::Tuple<IndexT, CoreGraphics::ConstantBufferId>>& surfaceBuffers = this->surfaceAllocator.Get<SurfaceBuffers>(sur)[*batchIt.val];
		Util::Array<Util::Tuple<IndexT, void*, SizeT>>& instanceBuffers = this->surfaceAllocator.Get<InstanceBuffers>(sur)[*batchIt.val];

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
					surfaceBuffers.Append(Util::MakeTuple(slot, buf));
				}
			}			
			else if (group == NEBULA_INSTANCE_GROUP && instanceTable != CoreGraphics::ResourceTableId::Invalid())
			{
				CoreGraphics::ConstantBufferId buf = CoreGraphics::GetGraphicsConstantBuffer(CoreGraphics::MainThreadConstantBuffer);
				if (buf != CoreGraphics::ConstantBufferId::Invalid())
				{
					SizeT bufSize = CoreGraphics::ShaderGetConstantBufferSize(shd, j);
					CoreGraphics::ResourceTableSetConstantBuffer(instanceTable, { buf, slot, 0, true, false, bufSize, 0 });

					// allocate new intermediate buffer, which will be copied to the constant memory on apply
					byte* buf = n_new_array(byte, bufSize);

					// add to surface
					instanceBuffers.Append(Util::MakeTuple(slot, buf, bufSize));
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
			if (constant.group == NEBULA_BATCH_GROUP)
			{
				surConst.instanceConstant = false;
				// go through surface-level buffers to find slot which matches
				IndexT k;
				for (k = 0; k < surfaceBuffers.Size(); k++)
				{
					if (Util::Get<0>(surfaceBuffers[k]) == constant.slot)
					{
						surConst.bufferIndex = k;
						surConst.buffer = Util::Get<1>(surfaceBuffers[k]);
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
					if (Util::Get<0>(instanceBuffers[k]) == constant.slot)
					{
						surConst.bufferIndex = k;
						surConst.mem = Util::Get<1>(instanceBuffers[k]);
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
	this->surfaceAllocator.Dealloc(sur.id);
}

//------------------------------------------------------------------------------
/**
*/
SurfaceInstanceId 
MaterialType::CreateSurfaceInstance(const SurfaceId id)
{
	Ids::Id32 inst = this->surfaceInstanceAllocator.Alloc();

	this->surfaceInstanceAllocator.Get<SurfaceInstanceConstants>(inst).Resize(this->batchToIndexMap.Size());

	auto batchIt = this->batchToIndexMap.Begin();
	while (batchIt != this->batchToIndexMap.End())
	{
		// get surface level stuff
		const Util::Array<SurfaceConstant>& constants = this->surfaceAllocator.Get<Constants>(id.id)[*batchIt.val];

		// get instance level stuff
		Util::FixedArray<SurfaceInstanceConstant>& surfaceInstanceConstants = this->surfaceInstanceAllocator.Get<SurfaceInstanceConstants>(inst)[*batchIt.val];

		const Util::Array<Util::Tuple<IndexT, void*, SizeT>>& buffers = this->surfaceAllocator.Get<InstanceBuffers>(id.id)[*batchIt.val];
		SizeT numBuffers = buffers.Size();
		this->surfaceInstanceAllocator.Get<SurfaceInstanceOffsets>(inst).Resize(numBuffers);

		// resize 
		surfaceInstanceConstants.Resize(constants.Size());
		for (IndexT i = 0; i < constants.Size(); i++)
		{
			surfaceInstanceConstants[i] = { constants[i].binding, constants[i].mem };
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
	this->surfaceInstanceAllocator.Dealloc(id.instance);
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
		if (constant.buffer != CoreGraphics::ConstantBufferId::Invalid() && constant.binding != UINT_MAX)
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
		if (constant.binding != UINT_MAX)
			memcpy((void*)constant.mem, value.AsVoidPtr(), value.Size());
		it++;
	}
}

//------------------------------------------------------------------------------
/**
*/
bool
MaterialType::BeginBatch(CoreGraphics::BatchGroup::Code batch)
{
	n_assert(this->currentBatch == CoreGraphics::BatchGroup::InvalidBatchGroup);
	n_assert(this->currentBatchIndex == InvalidIndex);
	IndexT idx = this->batchToIndexMap[batch];
	if (idx != InvalidIndex)
	{
		CoreGraphics::SetShaderProgram(this->programs[idx]);
		this->currentBatch = batch;
		this->currentBatchIndex = idx;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::ApplySurface(const SurfaceId id)
{
	n_assert(this->currentBatch != CoreGraphics::BatchGroup::InvalidBatchGroup);
	n_assert(this->currentBatchIndex != InvalidIndex);
	CoreGraphics::SetResourceTable(this->surfaceAllocator.Get<SurfaceTable>(id.id)[this->currentBatchIndex], NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::ApplyInstance(const SurfaceInstanceId id)
{
	n_assert(this->currentBatch != CoreGraphics::BatchGroup::InvalidBatchGroup);
	n_assert(this->currentBatchIndex != InvalidIndex);
	const CoreGraphics::ResourceTableId table = this->surfaceAllocator.Get<InstanceTable>(id.surface)[this->currentBatchIndex];
	if (table != CoreGraphics::ResourceTableId::Invalid())
	{
		// update global buffer, save new offsets, and apply table
		const Util::Array<Util::Tuple<IndexT, void*, SizeT>>& buffers = this->surfaceAllocator.Get<InstanceBuffers>(id.surface)[this->currentBatchIndex];
		Util::FixedArray<uint>& offsets = this->surfaceInstanceAllocator.Get<SurfaceInstanceOffsets>(id.instance);
		for (IndexT i = 0; i < buffers.Size(); i++)
		{
			offsets[i] = CoreGraphics::SetGraphicsConstants(CoreGraphics::MainThreadConstantBuffer, (byte*)Util::Get<1>(buffers[i]), Util::Get<2>(buffers[i]));
		}
		CoreGraphics::SetResourceTable(table, NEBULA_INSTANCE_GROUP, CoreGraphics::GraphicsPipeline, offsets);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialType::EndBatch()
{
	this->currentBatchIndex = InvalidIndex;
	this->currentBatch = CoreGraphics::BatchGroup::InvalidBatchGroup;
}


} // namespace Materials
