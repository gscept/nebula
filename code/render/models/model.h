#pragma once
//------------------------------------------------------------------------------
/**
	A model resource consists of nodes, each of which inhibit some information
	read from an .n3 file. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "models/nodes/modelnode.h"
#include "resources/resourceid.h"
#include "models/modelserver.h"
namespace Models
{

/// create model
const ModelId CreateModel();
/// destroy model
void DestroyModel(const ModelId id);

/// find model hierarchically
const ModelNodeId ModelFindNode(const ModelId model, const Util::StringAtom& name);
/// get bounding box
Math::bbox& ModelGetBoundingBox(const ModelId model);

typedef Ids::IdAllocator<
	Math::bbox,											// bounding box of entire model
	Util::Dictionary<Util::StringAtom, ModelNodeId>,	// nodes sorted by name
	ModelNodeId,										// root node
	Util::Array<Resources::ResourceId>					// resources loaded and contained in this model
> ModelAllocatorType;
extern ModelAllocatorType modelAllocator;

class StreamModelPool;
extern StreamModelPool* modelPool;
} // namespace Models