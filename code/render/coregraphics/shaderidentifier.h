#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShaderIdentifier
  
    The ShaderIdentifier is a zero-indexed registry which is used to 
    identify shaders in a constant time (instead of using dictionaries)
    which can be used to select variables and whatnot using a FixedArray,
    which is guaranteed to retain the same index during the execution of
    the application.

    Each shader resource will have its own shader identifier code,
    meaning we are limited to having MaxNumShaderIdentifiers different
    shader files before this class stops behaving (although this value can 
    be increased if need be)

    (C) 2015-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
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
class ShaderIdentifier
{
public:
    /// human readable name of a ModelNodeType
    typedef Util::StringAtom Name;
    /// binary code for a ModelNodeType
    typedef IndexT Code;

    /// convert from string
    static Code FromName(const Name& name);
    /// convert to string
    static Name ToName(Code c);
    /// maximum number of different ModelNodeTypes
    static const IndexT MaxNumShaderIdentifiers = 128;
    /// invalid model node type code
    static const IndexT InvalidBatchType = InvalidIndex;

private:
    friend class Base::ShaderServerBase;

    /// constructor
    ShaderIdentifier();

    Util::Dictionary<Name, IndexT> nameToCode;
    Util::Array<Name> codeToName;
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

