#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::BatchAttributes
    
    Singleton which handles batch attributes
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/singleton.h"
#include "core/refcounted.h"
#include "animsplitterhelper.h"
#include "skinhelper.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class BatchAttributes : public Core::RefCounted
{
	__DeclareClass(BatchAttributes);
	__DeclareSingleton(BatchAttributes);
public:
	/// constructor
	BatchAttributes();
	/// destructor
	virtual ~BatchAttributes();

	/// opens the batch attributes
	void Open();
	/// closes the batch attributes
	void Close();
	/// returns true if batch attributes are open
	bool IsOpen() const;

	/// returns splitter for resource name
	const Ptr<AnimSplitterHelper>& GetSplitter(const Util::String& resource);
	/// returns true if resource has splitter
	bool HasSplitter(const Util::String& resource);
	/// returns list of skins
	const Ptr<SkinHelper>& GetSkin(const Util::String& resource);
	/// returns true if resource has skins
	bool HasSkin(const Util::String& resource);
private:
	/// loads batch attributes
	void Load();

	Util::Dictionary<Util::String, Ptr<AnimSplitterHelper> > animSplitters;
	Util::Dictionary<Util::String, Ptr<SkinHelper> > skins;
	bool isOpen;
	
}; 

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<AnimSplitterHelper>& 
BatchAttributes::GetSplitter( const Util::String& resource )
{
	return this->animSplitters[resource];
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
BatchAttributes::HasSplitter( const Util::String& resource )
{
	return this->animSplitters.FindIndex(resource) >= 0;
}


//------------------------------------------------------------------------------
/**
*/
inline const Ptr<SkinHelper>& 
BatchAttributes::GetSkin(const Util::String& resource)
{
	return this->skins[resource];
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
BatchAttributes::HasSkin( const Util::String& resource )
{
	return this->skins.Contains(resource);
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------