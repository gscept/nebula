#pragma once
//------------------------------------------------------------------------------
/**
	A material type declares the draw steps and associated shaders

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/hashtable.h"
#include "coregraphics/batchgroup.h"
#include "coregraphics/shader.h"
#include "memory/chunkallocator.h"
#include "util/variant.h"
#include "util/fixedarray.h"
#include "util/arraystack.h"
#include "material.h"
#include "coregraphics/texture.h"
#include "coregraphics/constantbuffer.h"

namespace Materials
{
struct MaterialTexture
{
	Util::String name;
	CoreGraphics::TextureId defaultValue;
	CoreGraphics::TextureType type;
	bool system : 1;

	IndexT slot;
};
struct MaterialConstant
{
	Util::String name;
	Util::Variant defaultValue;
	Util::Variant min;
	Util::Variant max;
	Util::Variant::Type type;
	bool system : 1;

	CoreGraphics::ConstantBinding offset;
	IndexT slot;
	IndexT group;
};

class MaterialType
{
public:

	/// constructor
	MaterialType();
	/// destructor
	~MaterialType();

	/// setup after loading
	void Setup();

	/// create a surface from type
	SurfaceId CreateSurface();
	/// destroy surface
	void DestroySurface(SurfaceId mat);

	/// create an instance of a surface
	SurfaceInstanceId CreateSurfaceInstance(const SurfaceId id);
	/// destroy instance of a surface
	void DestroySurfaceInstance(const SurfaceInstanceId id);

	/// get constant index
	IndexT GetSurfaceConstantIndex(const SurfaceId sur, const Util::StringAtom& name);
	/// get texture index
	IndexT GetSurfaceTextureIndex(const SurfaceId sur, const Util::StringAtom& name);
	/// get surface constant instance index
	IndexT GetSurfaceConstantInstanceIndex(const SurfaceInstanceId sur, const Util::StringAtom& name);

	/// get default value for constant
	const Util::Variant GetSurfaceConstantDefault(const SurfaceId sur, IndexT idx);
	/// get default value for texture
	const CoreGraphics::TextureId GetSurfaceTextureDefault(const SurfaceId sur, IndexT idx);

	/// set constant in surface (applies to all instances)
	void SetSurfaceConstant(const SurfaceId sur, const IndexT idx, const Util::Variant& value);
	/// set texture in surface (applies to all instances)
	void SetSurfaceTexture(const SurfaceId sur, const IndexT idx, const CoreGraphics::TextureId tex);

	/// set instance constant
	void SetSurfaceInstanceConstant(const SurfaceInstanceId sur, const IndexT idx, const Util::Variant& value);

private:
	friend class MaterialServer;
	friend class SurfacePool;
	friend bool	MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch);
	friend bool	MaterialBeginSurface(const SurfaceId id);
	friend void	MaterialApplySurfaceInstance(const SurfaceInstanceId mat);
	friend void	MaterialEndSurface();
	friend void	MaterialEndBatch();

	/// apply type-specific material state
	bool BeginBatch(CoreGraphics::BatchGroup::Code batch);
	/// apply surface-level material state
	bool BeginSurface(const SurfaceId id);
	/// apply specific material instance, using the same batch as 
	void ApplyInstance(const SurfaceInstanceId mat);
	/// end surface-level material state
	void EndSurface();
	/// end batch
	void EndBatch();

	Util::HashTable<CoreGraphics::BatchGroup::Code, IndexT> batchToIndexMap;

	Util::Array<CoreGraphics::ShaderProgramId> programs;
	Util::Dictionary<Util::StringAtom, MaterialTexture> textures;
	Util::Dictionary<Util::StringAtom, MaterialConstant> constants;
	Util::Array<Util::Dictionary<Util::StringAtom, MaterialTexture>> texturesByBatch;
	Util::Array<Util::Dictionary<Util::StringAtom, MaterialConstant>> constantsByBatch;
	bool isVirtual;
	Util::String name;
	Util::String description;
	Util::String group;
	uint vertexType;

	CoreGraphics::BatchGroup::Code currentBatch;
	IndexT currentSurfaceBatchIndex;

	// the reason whe have an instance type is because it doesn't need the default value
	struct SurfaceInstanceConstant
	{
		CoreGraphics::ConstantBinding binding;
		CoreGraphics::ConstantBufferId buffer;
	};

	struct SurfaceConstant
	{
		Util::Variant defaultValue;
		IndexT bufferIndex;
		bool instanceConstant : 1;
		CoreGraphics::ConstantBinding binding;
		CoreGraphics::ConstantBufferId buffer;
	};

	struct SurfaceTexture
	{
		CoreGraphics::TextureId defaultValue;
		IndexT slot;
	};

	enum SurfaceMembers
	{
		SurfaceTable,
		InstanceTable,
		SurfaceBuffers,
		InstanceBuffers,
		Textures,
		Constants,
		TextureMap,
		ConstantMap
	};

	/// this will cause somewhat bad cache coherency, since the states across all passes are stored tightly/
	/// however, between two passes, the memory is still likely to have been nuked
	Ids::IdAllocator<
		Util::FixedArray<CoreGraphics::ResourceTableId>,										// surface level resource table, mapped batch -> table
		Util::FixedArray<CoreGraphics::ResourceTableId>,										// instance level resource table, mapped batch -> table
		Util::FixedArray<Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId>>>,		// surface level constant buffers, mapped batch -> buffers
		Util::FixedArray<Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId>>>,		// instance level instance buffers, mapped batch -> buffers
		Util::FixedArray<Util::Array<SurfaceTexture>>,											// textures
		Util::FixedArray<Util::Array<SurfaceConstant>>,											// constants
		Util::Dictionary<Util::StringAtom, IndexT>,												// name to resource map
		Util::Dictionary<Util::StringAtom, IndexT>												// name to constant map
	> surfaceAllocator;


	enum SurfaceInstanceMembers
	{
		SurfaceInstanceConstants,
		ConstantBufferOffsets,
		ConstantBufferInstance
	};
	Ids::IdAllocator<
		Util::FixedArray<Util::FixedArray<SurfaceInstanceConstant>>,	// copy of surface constants
		Util::FixedArray<Util::FixedArray<uint>>,						// offsets into descriptor
		Util::FixedArray<Util::FixedArray<uint>>						// instance to update
	> surfaceInstanceAllocator;
	MaterialTypeId id;
};

} // namespace Materials
