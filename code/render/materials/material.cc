//------------------------------------------------------------------------------
//  material.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "material.h"
#include "materialtype.h"
#include "materialpool.h"
namespace Materials
{

MaterialPool* materialPool = nullptr;
MaterialType* currentType = nullptr;

//------------------------------------------------------------------------------
/**
*/
bool
MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch)
{
	n_assert(type != nullptr);
	currentType = type;
	return currentType->BeginBatch(batch);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialApply(const Resources::ResourceId& mat)
{
	n_assert(materialPool != nullptr);
	currentType->ApplyInstance(materialPool->GetId(mat));
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialEndBatch()
{
	n_assert(currentType != nullptr);
	currentType->EndBatch();
	currentType = nullptr;
}

} // namespace Material
