//------------------------------------------------------------------------------
// modelloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelloader.h"
#include "model.h"

namespace Models
{

__ImplementClass(Models::ModelLoader, 'MOLO', Resources::ResourceLoader);
//------------------------------------------------------------------------------
/**
*/
ModelLoader::ModelLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelLoader::~ModelLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ModelLoader::Setup()
{
	this->resourceClass = Models::Model::RTTI;
	this->placeholderResourceId = "mdl:system/placeholder.n3";
	this->errorResourceId = "mdl:system/error.n3";
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::LoadStatus
ModelLoader::Load(const Ptr<Resources::Resource>& res)
{
	Ptr<Models::Model> mdl = res;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelLoader::Unload(const Ptr<Resources::Resource>& res)
{

}

} // namespace Models