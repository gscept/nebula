//------------------------------------------------------------------------------
// model.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "model.h"

namespace Models
{

StreamModelPool* modelPool;
ModelAllocatorType modelAllocator;


//------------------------------------------------------------------------------
/**
*/
const ModelId
CreateModel()
{
	Ids::Id32 id = modelAllocator.AllocObject();
	return ModelId(id);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyModel(const ModelId id)
{
	modelAllocator.Get<1>(id.id).Clear();
	modelAllocator.Get<3>(id.id).Clear();
	modelAllocator.DeallocObject(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const ModelId
FindNode(const ModelId model, const Util::StringAtom& name)
{
	const Util::Dictionary<Util::StringAtom, ModelNodeId>& dict = modelAllocator.Get<1>(model.id);
	return dict[name];
}

//------------------------------------------------------------------------------
/**
*/
const Math::bbox&
GetBoundingBox(const ModelId model)
{
	return modelAllocator.Get<0>(model.id);
}

} // namespace Models