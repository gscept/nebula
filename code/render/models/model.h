#pragma once
//------------------------------------------------------------------------------
/**
	A model resource consists of nodes, each of which inhibit some information
	read from an .n3 file. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "models/nodes/modelnode.h"
#include "math/bbox.h"
namespace Models
{

class StreamModelPool;
extern StreamModelPool* modelPool;

ID_32_TYPE(ModelId);

/// create model
const ModelId CreateModel();
/// destroy model
void DestroyModel(const ModelId id);

/// find model hierarchically
const ModelId FindNode(const ModelId model, const Util::StringAtom& name);
/// get bounding box
const Math::bbox& GetBoundingBox(const ModelId model);

typedef Ids::IdAllocator<
	Math::bbox,											// bounding box of entire model
	Util::Dictionary<Util::StringAtom, ModelNodeId>,	// nodes sorted by name
	ModelNodeId,										// root node
	Util::Array<Resources::ResourceId>					// resources loaded and contained in this model
> ModelAllocatorType;

extern ModelAllocatorType modelAllocator;

} // namespace Models