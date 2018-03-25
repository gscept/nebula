#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::SkinHelper
    
    Helps to setup skin assignment from XML
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/xmlreader.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class SkinHelper : public Core::RefCounted
{
	__DeclareClass(SkinHelper);
public:
	/// constructor
	SkinHelper();
	/// destructor
	virtual ~SkinHelper();

	/// sets up the skin helper
	void Setup(const Ptr<IO::XmlReader>& reader);

	/// gets skin by name
	const Util::Array<Util::String>& GetSkins() const;
	/// returns true if skin exists
	bool HasSkin(const Util::String& skin);
	
private:
	Util::Array<Util::String> skins;
}; 


//------------------------------------------------------------------------------
/**
*/
inline bool 
SkinHelper::HasSkin( const Util::String& skin )
{
	return this->skins.FindIndex(skin) != InvalidIndex;
}


//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>& 
SkinHelper::GetSkins() const
{
	return this->skins;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------