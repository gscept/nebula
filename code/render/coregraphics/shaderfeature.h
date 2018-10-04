#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShaderFeature
    
    A set of shader features is used to identify a specific variation of
    a shader. For performance reasons, several shader features are
    combined into a bit mask, so that finding a matching variation
    can be done by bit-mask operations. Association of feature names
    to bit numbers must be done dynamically, since shader features are
    not hardcoded into Nebula, instead, applications can implement their
    own features without having to change the Nebula runtime.
    
    In string form, a features mask consists of the shader feature
    names, separated by '|' characters:

    "Depth|Skinning"
    "Alpha|Skinning|Unlit"

    An application may define up to 32 unique features (corresponding
    to the number of bits in an unsigned int).

    Note that all shader feature functionality is offered through
    ShaderServer methods (this has been done to automatically 
    ensure thread safety for the shader feature code).

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/stringatom.h"
#include "util/array.h"
#include "util/dictionary.h"

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

    static const IndexT maxId = 32;
    IndexT uniqueId;
    Util::Dictionary<Name, IndexT> stringToIndex;
    Util::Array<Name> indexToString;
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------


