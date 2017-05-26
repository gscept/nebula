#pragma once
//------------------------------------------------------------------------------
/**
    @class Materials::MaterialFeature
    
	Holds material features and can also convert a string of feature separated by '|' into a mask,
	which can be used to check if a mesh is compliant with a specific material.

	This is identical to CoreGraphics::ShaderFeature but is only used for material masking and compliance determination.
    
    
    (C) 2014 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "util/array.h"
#include "util/dictionary.h"

namespace Materials
{
class MaterialFeature
{
public:
	/// a shader feature bit mask
	typedef uint Mask;
	/// a single shader feature name
	typedef Util::StringAtom Name;

private:
	friend class MaterialServer;

	/// constructor
	MaterialFeature();
	/// generate a bit mask from a shader feature string
	Mask StringToMask(const Util::String& str);
	/// convert a bit mask into a shader feature string
	Util::String MaskToString(Mask mask);

	static const IndexT maxId = 32;
	IndexT uniqueId;
	Util::Dictionary<Name, IndexT> stringToIndex;
	Util::Array<Name> indexToString;
}; 
} // namespace Materials
//------------------------------------------------------------------------------