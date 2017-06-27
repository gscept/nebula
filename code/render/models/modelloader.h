#pragma once
//------------------------------------------------------------------------------
/**
	Implements a resource loader for models
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourceloader.h"
namespace Models
{
class ModelLoader : public Resources::ResourceLoader
{
	__DeclareClass(ModelLoader);
public:
	/// constructor
	ModelLoader();
	/// destructor
	virtual ~ModelLoader();

	/// setup resource loader, initiates the placeholder and error resources if valid
	void Setup();
private:

	/// load model
	LoadStatus Load(const Ptr<Resources::Resource>& res);
	/// unload model
	void Unload(const Ptr<Resources::Resource>& res);
};
} // namespace Models