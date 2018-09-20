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
#include "coregraphics/texture.h"
#include "material.h"
namespace Materials
{
struct MaterialTexture
{
	CoreGraphics::TextureId defaultValue;
	CoreGraphics::TextureType type;
	bool system : 1;
};
struct MaterialConstant
{
	Util::Variant defaultValue;
	Util::Variant min;
	Util::Variant max;
	Util::Variant::Type type;
	bool system : 1;
};

class MaterialType
{
public:

	/// constructor
	MaterialType();
	/// destructor
	~MaterialType();

	/// create an instance of a material
	MaterialId CreateInstance();
	/// destroy an instance of a material
	void DestroyInstance(MaterialId mat);
	/// set constant in material
	void SetConstant(MaterialId mat, Util::StringAtom name, const Util::Variant& constant);
	/// set texture in material
	void SetTexture(MaterialId mat, Util::StringAtom, const CoreGraphics::TextureId tex);


private:
	friend class MaterialServer;
	friend class MaterialPool;
	friend void	MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch);
	friend void	MaterialApply(const Resources::ResourceId& mat);
	friend void	MaterialEndBatch();

	/// apply type-specific material state
	void BeginBatch(CoreGraphics::BatchGroup::Code batch);
	/// apply specific material instance, using the same batch as 
	void ApplyInstance(const MaterialId& mat);
	/// end batch
	void EndBatch();

	Util::Array<CoreGraphics::BatchGroup::Code> batches;
	Util::HashTable<CoreGraphics::BatchGroup::Code, CoreGraphics::ShaderProgramId> programs;
	Util::Dictionary<Util::StringAtom, MaterialTexture> textures;
	Util::Dictionary<Util::StringAtom, MaterialConstant> constants;
	bool isVirtual;
	Util::String name;
	Util::String description;
	Util::String group;
	uint vertexType;

	typedef Ids::IdAllocator<CoreGraphics::ShaderStateId> MaterialStateAllocator;
	Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<CoreGraphics::ShaderStateId>> states;
	Util::Array<CoreGraphics::ShaderStateId>* currentAllocator;

	/// this will cause somewhat bad cache coherency, since the states across all passes are stored tightly/
	/// however, between two passes, the memory is still likely to have been nuked
	Ids::IdAllocator<
		Util::Array<Ids::Id32>,		// state indices
		Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>>, // constants
		Util::Array<Util::HashTable<Util::StringAtom, CoreGraphics::ShaderConstantId>> // textures
	> materialAllocator;
	MaterialTypeId id;
};

} // namespace Materials
