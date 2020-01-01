#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShaderFeature
    
    Wrapper class for shader permutations by name. 
	Permutations contain several flags or'ed with | to 
	indicate shader functionality as modules. Strings
	are converted to hashes and stored in a hash table for
	reverse lookup.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/stringatom.h"
#include "util/hashtable.h"

namespace Base
{
    class ShaderServerBase;
}

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class ShaderFeature
{
public:
    /// a shader feature bit mask
    typedef uint Mask;
    /// a single shader feature name
    typedef Util::StringAtom Name;

private:
    friend class Base::ShaderServerBase;

    /// constructor
    ShaderFeature();
    /// generate a bit mask from a shader feature string
    Mask StringToMask(const Util::String& str);
    /// convert a bit mask into a shader feature string
    Util::String MaskToString(Mask mask);

	Util::HashTable<Mask, Name> nameHash;
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------


