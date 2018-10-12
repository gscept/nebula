#pragma once
//------------------------------------------------------------------------------
/**
	Materials represent a set of settings and a correlated type, which tells the engine which shader to use and how

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "resources/resourceid.h"
#include "coregraphics/batchgroup.h"
#include <functional>
namespace Materials
{

class MaterialType;
ID_32_TYPE(MaterialTypeId);
ID_32_TYPE(MaterialInstanceId);


/// begin a material batch
bool MaterialBeginBatch(MaterialType* type, CoreGraphics::BatchGroup::Code batch);
/// apply instance of material
void MaterialApply(const Resources::ResourceId& mat);
/// end a material batch
void MaterialEndBatch();

extern MaterialType* currentType;

class MaterialPool;
extern MaterialPool* materialPool;
} // namespace Materials
