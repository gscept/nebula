#pragma once
//------------------------------------------------------------------------------
/**
	Graphics context is a helper class to setup graphics contexts.

	A graphics context is a resource which holds a contextual representation for
	a graphics entity.

	Use the DeclareRegistration macro in the header and DefineRegistration in the implementation
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "timing/time.h"
#include "ids/id.h"
#include "ids/idallocator.h"			// include this here since it will be used by all contexts
#include "ids/idgenerationpool.h"
#include "util/stringatom.h"
#include "graphicsentity.h"

#define _DeclareContext() \
private:\
	static Graphics::GraphicsContext::State __state;\
	static Graphics::GraphicsContextFunctionBundle __bundle;\
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
	__state.Dealloc(__state.entitySliceMap.ValueAtIndex(id, i));\
	__state.entitySliceMap.Erase(i);\
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
	return __state.entitySliceMap[id];\
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
	__state.entitySliceMap = Util::HashTable<Graphics::GraphicsEntityId, Graphics::ContextEntityId, 64>(512);


namespace Graphics
{

class View;
class Stage;
struct GraphicsContextFunctionBundle
{
	void(*OnBeforeFrame)(const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnWaitForWork)(const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnBeforeView)(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnAfterView)(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnAfterFrame)(const IndexT frameIndex, const Timing::Time frameTime);

    void(*OnStageCreated)(const Ptr<Graphics::Stage>& stage);
    void(*OnDiscardStage)(const Ptr<Graphics::Stage>& stage);
    void(*OnViewCreated)(const Ptr<Graphics::View>& view);
    void(*OnDiscardView)(const Ptr<Graphics::View>& view);
    void(*OnAttachEntity)(Graphics::GraphicsEntityId entity);
    void(*OnRemoveEntity)(Graphics::GraphicsEntityId entity);
    void(*OnWindowResized)(IndexT windowId, SizeT width, SizeT height);
    void(*OnRenderAsPlugin)(const IndexT frameIndex, const Timing::Time frameTime, const Util::StringAtom& filter);

	GraphicsContextFunctionBundle() : OnBeforeFrame(nullptr), OnWaitForWork(nullptr), OnBeforeView(nullptr), OnAfterView(nullptr), OnAfterFrame(nullptr),
        OnStageCreated(nullptr), OnDiscardStage(nullptr), OnViewCreated(nullptr), OnDiscardView(nullptr), OnAttachEntity(nullptr), OnRemoveEntity(nullptr), OnWindowResized(nullptr), OnRenderAsPlugin(nullptr)
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

	// gcc doesnt like these at all.
	/// create context
	//virtual void Create() = 0;
	/// destroy context
	//virtual void Destroy() = 0;

protected:
	friend class GraphicsServer;


	struct State
	{
		Util::HashTable<GraphicsEntityId, ContextEntityId, 64> entitySliceMap;
		ContextEntityId(*Alloc)();
		void(*Dealloc)(ContextEntityId id);

	}* state;
};

} // namespace Graphics