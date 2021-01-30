#pragma once
//------------------------------------------------------------------------------
/**
    @struct Graphics::GraphicsContextState
    
    A graphics context is a resource which holds a contextual representation for
    a graphics entity.

    Use the __DeclareContext macro in the header and __ImplementContext in the implementation.

    @note
    The reason for why the function bundle and state are implemented through macros, is because
    they have to be static, and thus implemented explicitly once per each context.
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "timing/time.h"
#include "ids/id.h"
#include "ids/idallocator.h"            // include this here since it will be used by all contexts
#include "ids/idgenerationpool.h"
#include "util/stringatom.h"
#include "util/arraystack.h"
#include "graphicsentity.h"
#include "coregraphics/window.h"

#define __DeclarePluginContext() \
private:\
    static Graphics::GraphicsContextState __state;\
    static Graphics::GraphicsContextFunctionBundle __bundle;\
public:\
    static void RegisterEntity(const Graphics::GraphicsEntityId id);\
    static void DeregisterEntity(const Graphics::GraphicsEntityId id);\
    static bool IsEntityRegistered(const Graphics::GraphicsEntityId id);\
    static void Destroy(); \
    static Graphics::ContextEntityId GetContextId(const Graphics::GraphicsEntityId id); \
    static const Graphics::ContextEntityId& GetContextIdRef(const Graphics::GraphicsEntityId id); \
    static void BeginBulkRegister(); \
    static void EndBulkRegister(); \
private:

#define __DeclareContext() \
    __DeclarePluginContext(); \
    static void Defragment();


#define __ImplementPluginContext(ctx) \
Graphics::GraphicsContextState ctx::__state; \
Graphics::GraphicsContextFunctionBundle ctx::__bundle; \
void ctx::RegisterEntity(const Graphics::GraphicsEntityId id) \
{\
    Graphics::GraphicsContext::InternalRegisterEntity(id, std::forward<Graphics::GraphicsContextState>(__state));\
}\
\
void ctx::DeregisterEntity(const Graphics::GraphicsEntityId id)\
{\
    Graphics::GraphicsContext::InternalDeregisterEntity(id, std::forward<Graphics::GraphicsContextState>(__state));\
}\
\
bool ctx::IsEntityRegistered(const Graphics::GraphicsEntityId id)\
{\
    return __state.entitySliceMap.Contains(id);\
}\
void ctx::Destroy()\
{\
    Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);\
}\
Graphics::ContextEntityId ctx::GetContextId(const Graphics::GraphicsEntityId id)\
{\
    IndexT idx = __state.entitySliceMap.FindIndex(id); \
    if (idx == InvalidIndex) return Graphics::InvalidContextEntityId; \
    else return __state.entitySliceMap.ValueAtIndex(id, idx); \
}\
const Graphics::ContextEntityId& ctx::GetContextIdRef(const Graphics::GraphicsEntityId id)\
{\
    IndexT idx = __state.entitySliceMap.FindIndex(id); \
    n_assert(idx != InvalidIndex); \
    return __state.entitySliceMap.ValueAtIndex(id, idx); \
}\
void ctx::BeginBulkRegister()\
{\
    __state.entitySliceMap.BeginBulkAdd();\
}\
void ctx::EndBulkRegister()\
{\
    __state.entitySliceMap.EndBulkAdd();\
}

#define __ImplementContext(ctx, idAllocator) \
__ImplementPluginContext(ctx);\
void ctx::Defragment()\
{\
    Graphics::GraphicsContext::InternalDefragment(idAllocator, std::forward<Graphics::GraphicsContextState>(__state));\
}

#define __CreatePluginContext() \
    __state.Alloc = Alloc; \
    __state.Dealloc = Dealloc; \
    __state.currentStage = Graphics::NoStage;

#define __CreateContext() \
    __CreatePluginContext() \
    __state.Defragment = Defragment;

namespace Graphics
{

class View;
class Stage;
struct FrameContext;

enum StageBits
{
    NoStage                     = 1 << 0,
    OnBeginStage                = 1 << 1,
    OnPrepareViewStage          = 1 << 2,
    OnUpdateViewResourcesStage  = 1 << 3,
    OnUpdateResourcesStage      = 1 << 4,
    OnBeforeFrameStage          = 1 << 5,
    OnWaitForWorkStage          = 1 << 6,
    OnWorkFinishedStage         = 1 << 7,
    OnBeforeViewStage           = 1 << 8,
    OnAfterViewStage            = 1 << 9,
    OnAfterFrameStage           = 1 << 10,

    AllStages = OnBeginStage | OnPrepareViewStage | OnBeforeFrameStage | OnWaitForWorkStage | OnBeforeViewStage | OnAfterViewStage | OnAfterFrameStage
};
__ImplementEnumBitOperators(StageBits);

struct GraphicsContextFunctionBundle
{
    // frame stages
    void(*OnBegin)(const Graphics::FrameContext& ctx);
    void(*OnPrepareView)(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    void(*OnUpdateViewResources)(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    void(*OnUpdateResources)(const Graphics::FrameContext& ctx);
    void(*OnBeforeFrame)(const Graphics::FrameContext& ctx);
    void(*OnWaitForWork)(const Graphics::FrameContext& ctx);
    void(*OnWorkFinished)(const Graphics::FrameContext& ctx);
    void(*OnBeforeView)(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    void(*OnAfterView)(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    void(*OnAfterFrame)(const Graphics::FrameContext& ctx);

    // debug callbacks
    void(*OnRenderDebug)(uint32_t flags);

    // change callbacks
    void(*OnStageCreated)(const Ptr<Graphics::Stage>& stage);
    void(*OnDiscardStage)(const Ptr<Graphics::Stage>& stage);
    void(*OnViewCreated)(const Ptr<Graphics::View>& view);
    void(*OnDiscardView)(const Ptr<Graphics::View>& view);
    void(*OnAttachEntity)(Graphics::GraphicsEntityId entity);
    void(*OnRemoveEntity)(Graphics::GraphicsEntityId entity);
    void(*OnWindowResized)(const CoreGraphics::WindowId windowId, SizeT width, SizeT height);

    StageBits* StageBits;
    GraphicsContextFunctionBundle() : OnBegin(nullptr), OnPrepareView(nullptr), OnUpdateViewResources(nullptr), OnUpdateResources(nullptr),
        OnBeforeFrame(nullptr), OnWaitForWork(nullptr), OnWorkFinished(nullptr), OnBeforeView(nullptr), OnAfterView(nullptr), OnAfterFrame(nullptr),
        OnStageCreated(nullptr), OnDiscardStage(nullptr), OnViewCreated(nullptr), OnDiscardView(nullptr), OnAttachEntity(nullptr), OnRemoveEntity(nullptr), OnWindowResized(nullptr),
        StageBits(nullptr), OnRenderDebug(nullptr)
    {
    };
};

ID_32_TYPE(ContextEntityId)

struct GraphicsContextState
{
    StageBits currentStage; // used by the GraphicsServer to set the state
    StageBits allowedRemoveStages = StageBits::NoStage; // if a delete is done while not in one of these stages, it will be added as a deferred delete
    Util::ArrayStack<GraphicsEntityId, 8> delayedRemoveQueue;

    Util::Array<GraphicsEntityId> entities; // ContextEntityId -> GraphicsEntityId. kept adjacent to allocator data.
    Util::HashTable<GraphicsEntityId, ContextEntityId, 128, 64> entitySliceMap;
    ContextEntityId(*Alloc)();
    void(*Dealloc)(ContextEntityId id);
    void(*Defragment)();
    /// called after a context entity has moved index
    void(*OnInstanceMoved)(uint32_t toIndex, uint32_t fromIndex);
    /// called to manually handle fragmentation
    void(*OnDefragment)(uint32_t toIndex, uint32_t fromIndex);

    void CleanupDelayedRemoveQueue()
    {
        while (!this->delayedRemoveQueue.IsEmpty())
        {
            Graphics::GraphicsEntityId eid = this->delayedRemoveQueue[0];
            IndexT index = this->entitySliceMap.FindIndex(eid);
            n_assert(index != InvalidIndex);
            auto cid = this->entitySliceMap.ValueAtIndex(eid.id, index);
            this->Dealloc(cid);
            this->entitySliceMap.EraseIndex(eid, index);
            this->delayedRemoveQueue.EraseIndexSwap(0);
        }
    }
};

class GraphicsContext
{

public:
    /// constructor
    GraphicsContext();
    /// destructor
    virtual ~GraphicsContext();

protected:
    friend class GraphicsServer;
    static void InternalRegisterEntity(const Graphics::GraphicsEntityId id, Graphics::GraphicsContextState&& state);
    static void InternalDeregisterEntity(const Graphics::GraphicsEntityId id, Graphics::GraphicsContextState&& state);
    template<class ID_ALLOCATOR>
    static void InternalDefragment(ID_ALLOCATOR& allocator, Graphics::GraphicsContextState&& state);
};

//------------------------------------------------------------------------------
/**
*/
template<class ID_ALLOCATOR>
inline void
GraphicsContext::InternalDefragment(ID_ALLOCATOR& allocator, Graphics::GraphicsContextState&& state)
{
    auto& freeIds = allocator.FreeIds();
    uint32_t index;
    uint32_t oldIndex;
    Graphics::GraphicsEntityId lastId;
    IndexT mapIndex;
    uint32_t dataSize;
    SizeT size = freeIds.Size();
    for (SizeT i = size - 1; i >= 0; --i)
    {
        index = freeIds.Back();
        freeIds.EraseBack();
        dataSize = (uint32_t)allocator.Size();
        if (index >= dataSize)
        {
            continue;
        }
        oldIndex = dataSize - 1;
        lastId = state.entities[oldIndex].id;
        if (state.OnInstanceMoved != nullptr)
            state.OnInstanceMoved(index, oldIndex);
        allocator.EraseIndexSwap(index);
        state.entities.EraseIndexSwap(index);
        mapIndex = state.entitySliceMap.FindIndex(lastId);
        if (mapIndex != InvalidIndex)
        {
            state.entitySliceMap.ValueAtIndex(lastId, mapIndex) = index;
        }
        else
        {
            freeIds.Append(index);
            i++;
        }
    }
    freeIds.Clear();
}

} // namespace Graphics
