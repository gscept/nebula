#pragma once
//------------------------------------------------------------------------------
/**
	Graphics context is a helper class to setup graphics contexts.

	A graphics context is a resource which holds a contextual representation for
	a graphics entity.

	Use the DeclareRegistration macro in the header and DefineRegistration in the implementation.

	The reason for why the function bundle and state are implemented through macros, is because
	they have to be static, and thus implemented explicitly once per each context.
	
	(C)2017-2018 Individual contributors, see AUTHORS file
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

#define _DeclareContext() \
private:\
	static Graphics::GraphicsContext::State __state;\
	static Graphics::GraphicsContextFunctionBundle __bundle;\
	static void GarbageCollect();\
public:\
	static void RegisterEntity(const Graphics::GraphicsEntityId id);\
	static void DeregisterEntity(const Graphics::GraphicsEntityId id);\
	static bool IsEntityRegistered(const Graphics::GraphicsEntityId id);\
	static void Destroy(); \
	static Graphics::ContextEntityId GetContextId(const Graphics::GraphicsEntityId id); \
	static void BeginBulkRegister(); \
	static void EndBulkRegister();

#define _ImplementContext(ctx) \
Graphics::GraphicsContext::State ctx::__state; \
Graphics::GraphicsContextFunctionBundle ctx::__bundle; \
void ctx::GarbageCollect() \
{\
	while (!ctx::__state.delayedRemoveQueue.IsEmpty())\
	{\
		Graphics::GraphicsEntityId eid = ctx::__state.delayedRemoveQueue[0];\
		IndexT index = __state.entitySliceMap.FindIndex(eid);\
		n_assert(index != InvalidIndex);\
		__state.Dealloc(__state.entitySliceMap.ValueAtIndex(eid.id, index));\
		__state.entitySliceMap.EraseIndex(eid, index);\
		ctx::__state.delayedRemoveQueue.EraseIndexSwap(0);\
	}\
}\
void ctx::RegisterEntity(const Graphics::GraphicsEntityId id) \
{\
	n_assert(!__state.entitySliceMap.Contains(id));\
	Graphics::ContextEntityId allocId = __state.Alloc();\
	__state.entitySliceMap.Add(id, allocId);\
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

#define _CreateContext() \
	__state.Alloc = Alloc; \
	__state.Dealloc = Dealloc; \
	__state.currentStage = Graphics::NoStage; \
	__bundle.GarbageCollect = GarbageCollect;

namespace Graphics
{

class View;
class Stage;

enum StageBits
{
	NoStage,
	OnBeforeFrameStage,
	OnWaitForWorkStage,
	OnBeforeViewStage,
	OnAfterViewStage,
	OnAfterFrameStage,

	AllStages = OnBeforeFrameStage | OnWaitForWorkStage | OnBeforeViewStage | OnAfterViewStage | OnAfterFrameStage
};
__ImplementEnumBitOperators(StageBits);

struct GraphicsContextFunctionBundle
{
	// frame stages
	void(*OnBeforeFrame)(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks);
	void(*OnWaitForWork)(const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnBeforeView)(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnAfterView)(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnAfterFrame)(const IndexT frameIndex, const Timing::Time frameTime);

    // debug callbacks
    void(*OnRenderDebug)(uint32_t flags);

	// change callbacks
    void(*OnStageCreated)(const Ptr<Graphics::Stage>& stage);
    void(*OnDiscardStage)(const Ptr<Graphics::Stage>& stage);
    void(*OnViewCreated)(const Ptr<Graphics::View>& view);
    void(*OnDiscardView)(const Ptr<Graphics::View>& view);
    void(*OnAttachEntity)(Graphics::GraphicsEntityId entity);
    void(*OnRemoveEntity)(Graphics::GraphicsEntityId entity);
    void(*OnWindowResized)(IndexT windowId, SizeT width, SizeT height);

	// Called before frame
	void(*GarbageCollect)();

	// frame script callbacks
    void(*OnRenderAsPlugin)(const IndexT frameIndex, const Timing::Time frameTime, const Util::StringAtom& filter);

	StageBits* StageBits;
	GraphicsContextFunctionBundle() : OnBeforeFrame(nullptr), OnWaitForWork(nullptr), OnBeforeView(nullptr), OnAfterView(nullptr), OnAfterFrame(nullptr),
        OnStageCreated(nullptr), OnDiscardStage(nullptr), OnViewCreated(nullptr), OnDiscardView(nullptr), OnAttachEntity(nullptr), OnRemoveEntity(nullptr), OnWindowResized(nullptr), OnRenderAsPlugin(nullptr),
		StageBits(nullptr), OnRenderDebug(nullptr), GarbageCollect(nullptr)
	{
	};
};


ID_32_TYPE(ContextEntityId)

class GraphicsContext
{

public:
	/// constructor
	GraphicsContext();
	/// destructor
	virtual ~GraphicsContext();

protected:
	friend class GraphicsServer;

	struct State
	{
		StageBits currentStage;	// used by the GraphicsServer to set the state
		StageBits allowedRemoveStages = StageBits::NoStage;	// if a delete is done while not in one of these stages, it will be added as a deferred delete
		Util::ArrayStack<GraphicsEntityId, 8> delayedRemoveQueue;

		Util::HashTable<GraphicsEntityId, ContextEntityId, 128, 64> entitySliceMap;
		ContextEntityId(*Alloc)();
		void(*Dealloc)(ContextEntityId id);
	};
};

} // namespace Graphics