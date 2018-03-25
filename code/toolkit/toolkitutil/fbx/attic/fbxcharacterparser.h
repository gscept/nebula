#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxCharacterParser
    
    Encapsulates character parsing
    
    (C) 2012 gscept
*/
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class FbxCharacterParser : public Core::RefCounted
{
	__DeclareClass(FbxCharacterParser);
public:
	/// constructor
	FbxCharacterParser();
	/// destructor
	virtual ~FbxCharacterParser();

private:
	Util::Array<Ptr<Character> > characters;
}; 
} // namespace ToolkitUtil
//------------------------------------------------------------------------------