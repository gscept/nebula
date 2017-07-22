#pragma once
//------------------------------------------------------------------------------
/**
	Implements a resource loader for models
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "util/stack.h"
namespace Models
{
class ModelNode;
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

	/// perform actual load, override in subclass
	LoadStatus Load(const Ptr<Resources::Resource>& res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload resource
	void Unload(const Ptr<Resources::Resource>& res);

	Util::Array<Util::StringAtom> pendingResources;
	Util::Stack<Ptr<ModelNode>> nodeStack;
};
} // namespace Models