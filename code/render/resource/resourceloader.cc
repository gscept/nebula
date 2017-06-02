//------------------------------------------------------------------------------
// resourceloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourceloader.h"

namespace Resources
{

__ImplementClass(Resources::ResourceLoader, 'RELO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceLoader::ResourceLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::~ResourceLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Setup()
{
	// implement loader-specific setups, such as placeholder and error resource ids, as well as the acceptable resource class
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Update(IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->pending.Size(); i++)
	{
		const _PendingResource& element = this->pending.ValueAtIndex(i);
		const Ptr<Resource>& res = element.res;

		// load resource, get status from load function
		LoadStatus status = this->Load(res);
		if (status != Delay)
		{
			if (status == Success)
			{
				element.success(res);
				this->usage.Add(res->resourceId, 1);
				this->loaded.Add(res->resourceId, res);

				// if tag is valid, group it under tag
				if (element.tag.IsValid())
				{
					IndexT idx = this->tagToIndexMap.FindIndex(element.tag);
					if (idx == InvalidIndex) this->tagToIndexMap.Add(element.tag, Util::Array<int>());
					
					// add indexed key to loaded map so that tag removal can find them from the loaded list
					this->tagToIndexMap.ValueAtIndex(idx).Append(this->loaded.FindIndex(res->resourceId));
				}
			}
			else if (status == Failed)	element.failed(res);
			this->pending.EraseAtIndex(i--);
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus
ResourceLoader::Load(const Ptr<Resource>& res)
{
	// override in subclass
	return Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Unload(const Ptr<Resource>& res)
{
	// override in subclass to perform actual resource unload
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::DiscardByTag(const Util::StringAtom& tag)
{
	const Util::Array<int>& indices = this->tagToIndexMap[tag];
	IndexT i;
	for (i = 0; i < indices.Size(); i++)
	{
		const int& idx = indices[i];
		this->Unload(this->loaded.ValueAtIndex(idx));
		this->loaded.EraseAtIndex(idx);
		this->usage.EraseAtIndex(idx);
	}

	// remove tag-index entry
	this->tagToIndexMap.Erase(tag);
}

} // namespace Resources