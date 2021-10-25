#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for models

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreamcache.h"
#include "util/stack.h"
#include "model.h"
#include "nodes/primitivenode.h"
#include "nodes/shaderstatenode.h"
#include "nodes/transformnode.h"
#include "nodes/characternode.h"
#include "nodes/characterskinnode.h"
#include "nodes/particlesystemnode.h"

namespace Visibility
{
class VisibilityContext;
}
namespace Graphics
{
struct FrameContext;
}
namespace Models
{

class ModelNode;
class ModelContext;
class StreamModelCache : public Resources::ResourceStreamCache
{
    __DeclareClass(StreamModelCache);
public:
    /// constructor
    StreamModelCache();
    /// destructor
    virtual ~StreamModelCache();

    /// setup resource loader, initiates the placeholder and error resources if valid
    void Setup() override;

    /// get node instances
    const Util::Dictionary<Util::StringAtom, Models::ModelNode*>& GetModelNodes(const ModelId id);

    /// get bounding box of model
    const Math::bbox& GetModelBoundingBox(const ModelId id) const;
    /// get bounding box of model
    Math::bbox& GetModelBoundingBox(const ModelId id);

private:
    friend class PrimitiveNode;
    friend class CharacterNode;
    friend class CharacterSkinNode;
    friend class ParticleSystemNode;
    friend class Models::ModelContext;
    friend class Visibility::VisibilityContext;

    /// perform actual load, override in subclass
    LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource
    void Unload(const Resources::ResourceId id);

    /// used for looking up constructors
    Util::Dictionary<Util::FourCC, Ids::Id8> nodeFourCCMapping;

    enum
    {
        ModelBoundingBox,
        ModelNodeAllocator,
        ModelNodes,
        RootNode
    };

    Ids::IdAllocator<
        Math::bbox,                                                 // 0 - total bounding box
        Memory::ArenaAllocator<MODEL_MEMORY_CHUNK_SIZE>,            // 1 - memory allocator
        Util::Dictionary<Util::StringAtom, Models::ModelNode*>,     // 2 - nodes
        Models::ModelNode*                                         // 3 - root
    > modelAllocator;
    __ImplementResourceAllocator(modelAllocator);

    Util::Array<std::function<Models::ModelNode*(Memory::ArenaAllocator<MODEL_MEMORY_CHUNK_SIZE>&)>> nodeConstructors;

    static Ids::Id8 NodeMappingCounter;

#define IMPLEMENT_NODE_ALLOCATOR(FourCC, Type, NodeList) \
    nodeConstructors.Append([this](Memory::ArenaAllocator<MODEL_MEMORY_CHUNK_SIZE>& alloc) -> Models::ModelNode* { return alloc.Alloc<Models::Type>(); }); \
    this->nodeFourCCMapping.Add(FourCC, NodeMappingCounter++);
};

/// get node instances
const Util::Dictionary<Util::StringAtom, Models::ModelNode*>& ModelGetNodes(const ModelId id);

} // namespace Models
