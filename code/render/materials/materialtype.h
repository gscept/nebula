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
#include "ids/idallocator.h"
namespace Materials
{
struct MaterialTexture
{
	CoreGraphics::TextureId default;
	CoreGraphics::TextureType type;
};
struct MaterialConstant
{
	Util::Variant default;
	Util::Variant min;
	Util::Variant max;
	Util::Variant::Type type;
};

struct MaterialSetup
{
	Util::Array<Util::StringAtom> textureNames;
	Util::Array<MaterialTexture> textures;
	Util::Array<Util::StringAtom> constantNames;
	Util::Array<MaterialConstant> constants;
};

ID_32_TYPE(MaterialTypeId);
class MaterialType
{
public:

	/// constructor
	MaterialType();
	/// destructor
	~MaterialType();

	/// create an instance of a material
	Ids::Id32 CreateMaterial();
	/// destroy an instance of a material
	void DestroyMaterial(Ids::Id32 mat);
	/// setup material
	void SetupMaterial(Ids::Id32 mat, MaterialSetup setup);
	/// set constant in material
	void MaterialSetConstant(Ids::Id32 mat, Util::StringAtom name, const Util::Variant& constant);
	/// set texture in material
	void MaterialSetTexture(Ids::Id32 mat, Util::StringAtom, const CoreGraphics::TextureId tex);

	/// apply type-specific material state
	void BeginBatch(CoreGraphics::BatchGroup::Code batch);
	/// end batch
	void EndBatch();

	/// apply specific material instance, using the same batch as 
	void ApplyMaterial(Ids::Id32 mat);
private:
	friend class MaterialServer;
	friend class MaterialPool;


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
