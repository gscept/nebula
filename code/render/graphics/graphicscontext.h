#pragma once
//------------------------------------------------------------------------------
/**
	Graphics context is a helper class to setup graphics contexts.

	A graphics context is a resource which holds a contextual representation for
	a graphics entity.

	Use the DeclareRegistration macro in the header and DefineRegistration in the implementation
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "timing/time.h"
#include "ids/id.h"
#include "ids/idallocator.h"			// include this here since it will be used by all contexts
#include "ids/idgenerationpool.h"
#include "graphicsentity.h"

#define DeclareContext() \
private:\
	static Graphics::GraphicsContext::State __state;\
	static Graphics::GraphicsContextFunctionBundle __bundle;\
public:\
	static void RegisterEntity(const Graphics::GraphicsEntityId id);\
	static void DeregisterEntity(const Graphics::GraphicsEntityId id);\
	static bool IsEntityRegistered(const Graphics::GraphicsEntityId id);\
	static void Destroy();

#define ImplementContext(ctx) \
Graphics::GraphicsContext::State ctx::__state; \
Graphics::GraphicsContextFunctionBundle ctx::__bundle; \
void ctx::RegisterEntity(const Graphics::GraphicsEntityId id) \
{\
	n_assert(!__state.entitySliceMap.Contains(id.id));\
	Graphics::ContextEntityId allocId = __state.Alloc();\
	__state.entitySliceMap.Add(id, allocId);\
}\
\
void ctx::DeregisterEntity(const Graphics::GraphicsEntityId id)\
{\
	IndexT i = __state.entitySliceMap.FindIndex(id.id);\
	n_assert(i != InvalidIndex);\
	__state.Dealloc(__state.entitySliceMap.ValueAtIndex(i));\
	__state.entitySliceMap.Erase(i);\
}\
\
bool ctx::IsEntityRegistered(const Graphics::GraphicsEntityId id)\
{\
	return __state.entitySliceMap.Contains(id.id);\
}\
void ctx::Destroy()\
{\
	Graphics::GraphicsServer::Instance()->UnregisterGraphicsContext(&__bundle);\
}

#define CreateContext() \
	__state.Alloc = Alloc; \
	__state.Dealloc = Dealloc; 

#define GetContextId(id) __state.entitySliceMap[id.id]


namespace Graphics
{

class View;
struct GraphicsContextFunctionBundle
{
	void(*OnBeforeFrame)(const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnVisibilityReady)(const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnBeforeView)(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnAfterView)(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	void(*OnAfterFrame)(const IndexT frameIndex, const Timing::Time frameTime);

	GraphicsContextFunctionBundle() : OnBeforeFrame(nullptr), OnVisibilityReady(nullptr), OnBeforeView(nullptr), OnAfterView(nullptr), OnAfterFrame(nullptr)
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

	/// create context
	virtual void Create() = 0;
	/// destroy context
	virtual void Destroy() = 0;

protected:
	friend class GraphicsServer;


	struct State
	{
		Util::Dictionary<GraphicsEntityId, ContextEntityId> entitySliceMap;
		ContextEntityId(*Alloc)();
		void(*Dealloc)(ContextEntityId id);
	}* state;
};

} // namespace Graphics