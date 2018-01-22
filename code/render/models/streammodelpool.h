#pragma once
//------------------------------------------------------------------------------
/**
	Implements a resource loader for models
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "util/stack.h"
#include "nodes/primitivenode.h"
#include "nodes/shaderstatenode.h"
#include "nodes/transformnode.h"
#include "nodes/characternode.h"
#include "nodes/characterskinnode.h"
#include "nodes/particlesystemnode.h"

namespace Models
{

enum NodeType
{
	TransformNodeType,
	PrimtiveNodeType,
	ShaderStateNodeType,
	CharacterSkinNodeType,
	CharacterNodeType,
	ParticleSystemNodeType,

	NumNodeTypes
};

class ModelNode;
class StreamModelPool : public Resources::ResourceStreamPool
{
	__DeclareClass(StreamModelPool);
public:
	/// constructor
	StreamModelPool();
	/// destructor
	virtual ~StreamModelPool();

	/// setup resource loader, initiates the placeholder and error resources if valid
	void Setup();

private:
	friend class PrimitiveNode;
	friend class CharacterNode;
	friend class CharacterSkinNode;
	friend class ParticleSystemNode;

	/// perform actual load, override in subclass
	LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Ids::Id24 id);

	typedef Util::KeyValuePair<Util::FourCC, Ids::Id32> NodeTypeId;
	Util::Stack<NodeTypeId> nodeStack;

	Util::Dictionary<Util::FourCC, std::function<Ids::Id32()>> constructors;
	Util::Dictionary<Util::FourCC, std::function<ModelNode*(Ids::Id32)>> accessors;
	Util::Array<TransformNode> transformNodes;
	Util::Array<PrimitiveNode> primitiveNodes;
	Util::Array<ShaderStateNode> shaderStatenodes;
	Util::Array<CharacterSkinNode> characterSkinNodes;
	Util::Array<CharacterNode> characterNodes;
	Util::Array<ParticleSystemNode> particleSystemNode;

	Ids::Id8 nodeTypeIndices[NumNodeTypes];

	Ids::IdAllocator<
		Math::bbox,
		Util::Dictionary<Util::StringAtom, NodeTypeId>
	> modelNodeAllocator;
	__ImplementResourceAllocator(modelNodeAllocator);
};
} // namespace Models