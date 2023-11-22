#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for models

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourceloader.h"
#include "util/stack.h"
#include "model.h"
#include "nodes/characterskinnode.h"
#include "nodes/particlesystemnode.h"

namespace Visibility
{
class VisibilityContext;
}
namespace Models
{

class ModelNode;
class ModelContext;
class ModelLoader : public Resources::ResourceLoader
{
    __DeclareClass(ModelLoader);
public:
    /// constructor
    ModelLoader();
    /// destructor
    virtual ~ModelLoader();

    /// setup resource loader, initiates the placeholder and error resources if valid
    void Setup() override;

private:
    friend class PrimitiveNode;
    friend class CharacterNode;
    friend class CharacterSkinNode;
    friend class ParticleSystemNode;
    friend class Models::ModelContext;
    friend class Visibility::VisibilityContext;

    /// perform actual load, override in subclass
    Resources::ResourceUnknownId InitializeResource(Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource
    void Unload(const Resources::ResourceId id);

    /// used for looking up constructors
    Util::Dictionary<Util::FourCC, std::function<Models::ModelNode* ()>> nodeConstructors;

#define IMPLEMENT_NODE_ALLOCATOR(FourCC, Type) \
    nodeConstructors.Add(FourCC, []() -> Models::ModelNode* { \
        Models::ModelNode* node = (Models::ModelNode*)Memory::Alloc(Memory::ObjectHeap, sizeof(Models::Type)); \
        new (node) Models::Type; \
        return node; \
    });

};

/// Get nodes
const Util::Array<Models::ModelNode*>& ModelGetNodes(const ModelId id);

} // namespace Models
