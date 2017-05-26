//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelcontext.h"

namespace Graphics
{

__ImplementClass(Graphics::ModelContext, 'MOCO', Graphics::GraphicsContext);
__ImplementSingleton(Graphics::ModelContext);
//------------------------------------------------------------------------------
/**
*/
ModelContext::ModelContext()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::~ModelContext()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
int64_t
ModelContext::RegisterEntity(const int64_t& entity)
{
	GraphicsContext::AllocateSlice<_Model>(entity, this->modelData, [](_Model& data, IndexT idx)
	{

	});

	GraphicsContext::AllocateSlice<_Staging>(entity, this->stagingData, [](_Staging& data, IndexT idx)
	{

	});
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::UnregisterEntity(const int64_t& slice)
{

}

} // namespace Graphics