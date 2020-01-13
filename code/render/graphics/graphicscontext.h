#pragma once
//------------------------------------------------------------------------------
/**
	Graphics context is a helper class to setup graphics contexts.

	A graphics context is a resource which holds a contextual representation for
	a graphics entity.

	Use the DeclareRegistration macro in the header and DefineRegistration in the implementation.

	The reason for why the function bundle and state are implemented through macros, is because
	they have to be static, and thus implemented explicitly once per each context.
	
	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "timing/time.h"
#include "ids/id.h"
#include "ids/idallocator.h"			// include this here since it will be used by all contexts
#include "ids/idgenerationpool.h"
#include "util/stringatom.h"
#include "util/arraystack.h"
#include "graphicsentity.h"
#include "coregraphics/window.h"


#define _DeclarePluginContext() \
private:\
	static Graphics::GraphicsContextState __state;\
	static Graphics::GraphicsContextFunctionBundle __bundle;\
public:\
	static void RegisterEntity(const Graphics::GraphicsEntityId id);\
	static void DeregisterEntity(const Graphics::GraphicsEntityId id);\
	static bool IsEntityRegistered(const Graphics::GraphicsEntityId id);\
	static void Destroy(); \
	static Graphics::ContextEntityId GetContextId(const Graphics::GraphicsEntityId id); \
	static void BeginBulkRegister(); \
	static void EndBulkRegister(); \
private:

#define _DeclareContext() \
	_DeclarePluginContext(); \
	static void Defragment();


#define _ImplementPluginContext(ctx) \
Graphics::GraphicsContextState ctx::__state; \
Graphics::GraphicsContextFunctionBundle ctx::__bundle; \
void ctx::RegisterEntity(const Graphics::GraphicsEntityId id) \
{\
	n_assert(!__state.entitySliceMap.Contains(id));\
	Graphics::ContextEntityId allocId = __state.Alloc();\
	__state.entitySliceMap.Add(id, allocId);\
	if ((unsigned)__state.entities.Size() <= allocId.id)\
	{\
		if ((unsigned)__state.entities.Capacity() <= allocId.id)\
		{\
			__state.entities.Grow();\
		}\
		__state.entities.resize(allocId.id + 1);\
	}\
	__state.entities[allocId.id] = id;\
}\
\
void ctx::DeregisterEntity(const Graphics::GraphicsEntityId id)\
{\
	IndexT i = __state.entitySliceMap.FindIndex(id);\
	n_assert(i != InvalidIndex);\
	if (__state.allowedRemoveStages & __state.currentStage) \
	{ \
		__state.Dealloc(__state.entitySliceMap.ValueAtIndex(id, i));\
		__state.entitySliceMap.EraseIndex(id, i);\
	} \
	else \
	{ \
		__state.delayedRemoveQueue.Append(id); \
	} \
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
	if (idx == InvalidIndex) return Graphics::ContextEntityId::Invalid(); \
	else return __state.entitySliceMap.ValueAtIndex(id, idx); \
}\
void ctx::BeginBulkRegister()\
{\
	__state.entitySliceMap.BeginBulkAdd();\
}\
void ctx::EndBulkRegister()\
{\
	__state.entitySliceMap.EndBulkAdd();\
}

#define _ImplementContext(ctx, idAllocator) \
_ImplementPluginContext(ctx);\
void ctx::Defragment()\
{\
	auto& freeIds = idAllocator.FreeIds();\
	uint32_t index;\
	uint32_t oldIndex;\
	Graphics::GraphicsEntityId lastId;\
	IndexT mapIndex;\
	uint32_t dataSize;\
	SizeT size = freeIds.Size();\
	for (SizeT i = size - 1; i >= 0; --i)\
	{\
		index = freeIds.Back();\
		freeIds.EraseBack();\
		dataSize = (uint32_t)idAllocator.Size();\
		if (index >= dataSize) { continue; }\
		oldIndex = dataSize - 1;\
		lastId = __state.entities[oldIndex].id;\
		idAllocator.EraseIndexSwap(index);\
		__state.entities.EraseIndexSwap(index);\
		mapIndex = __state.entitySliceMap.FindIndex(lastId);\
		if (mapIndex != InvalidIndex)\
		{\
			__state.entitySliceMap.ValueAtIndex(lastId, mapIndex) = index;\
		}\
		else\
		{\
			freeIds.Append(index);\
			i++;\
		}\
        if (__state.OnInstanceMoved != nullptr) \
        {\
            __state.OnInstanceMoved(index, oldIndex);\
        }\
	}\
	freeIds.Clear();\
}

#define _CreatePluginContext() \
	__state.Alloc = Alloc; \
	__state.Dealloc = Dealloc; \
	__state.currentStage = Graphics::NoStage;

#define _CreateContext() \
	_CreatePluginContext() \
	__state.Defragment = Defragment;

namespace Graphics
{

class View;
class Stage;
struct FrameContext;

enum StageBits
{
	NoStage,
	OnPrepareViewStage,
	OnBeforeFrameStage,
	OnWaitForWorkStage,
	OnBeforeViewStage,
	OnAfterViewStage,
	OnAfterFrameStage,

	AllStages = OnPrepareViewStage | OnBeforeFrameStage | OnWaitForWorkStage | OnBeforeViewStage | OnAfterViewStage | OnAfterFrameStage
};
__ImplementEnumBitOperators(StageBits);

struct GraphicsContextFunctionBundle
{
	// frame stages
	void(*OnPrepareView)(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
	void(*OnBeforeFrame)(const Graphics::FrameContext& ctx);
	void(*OnWaitForWork)(const Graphics::FrameContext& ctx);
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
	GraphicsContextFunctionBundle() : OnPrepareView(nullptr), OnBeforeFrame(nullptr), OnWaitForWork(nullptr), OnBeforeView(nullptr), OnAfterView(nullptr), OnAfterFrame(nullptr),
        OnStageCreated(nullptr), OnDiscardStage(nullptr), OnViewCreated(nullptr), OnDiscardView(nullptr), OnAttachEntity(nullptr), OnRemoveEntity(nullptr), OnWindowResized(nullptr),
		StageBits(nullptr), OnRenderDebug(nullptr)
	{
	};
};

ID_32_TYPE(ContextEntityId)

struct GraphicsContextState
{
	StageBits currentStage;	// used by the GraphicsServer to set the state
	StageBits allowedRemoveStages = StageBits::NoStage;	// if a delete is done while not in one of these stages, it will be added as a deferred delete
	Util::ArrayStack<GraphicsEntityId, 8> delayedRemoveQueue;

	Util::Array<GraphicsEntityId> entities; // ContextEntityId -> GraphicsEntityId. kept adjacent to allocator data.
	Util::HashTable<GraphicsEntityId, ContextEntityId, 128, 64> entitySliceMap;
	ContextEntityId(*Alloc)();
	void(*Dealloc)(ContextEntityId id);
	void(*Defragment)();
    /// called after a context entity has moved index
    void(*OnInstanceMoved)(uint32_t toIndex, uint32_t fromIndex);

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
};

} // namespace Graphics
