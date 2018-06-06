#pragma once
//------------------------------------------------------------------------------
/**
    @file attr/valuetype.h
    
    Defines the valid attribute value types as enum.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace Attr
{
enum ValueType
{
    VoidType = Util::Variant::Void,
    IntType = Util::Variant::Int,
    FloatType = Util::Variant::Float,
    BoolType = Util::Variant::Bool,
    Float4Type = Util::Variant::Float4,
    StringType = Util::Variant::String,
    Matrix44Type = Util::Variant::Matrix44,
	Transform44Type = Util::Variant::Transform44,
    BlobType = Util::Variant::Blob,
    GuidType = Util::Variant::Guid,
};

// these class definitions are just for template specializations later on,
// templates are also possible with enums, but they don't look very
// intuitive in the debugger...
class VoidTypeClass {};
class IntTypeClass {};
class FloatTypeClass {};
class BoolTypeClass {};
class Float4TypeClass {};
class StringTypeClass {};
class Matrix44TypeClass {};
class BlobTypeClass {};
class GuidTypeClass {};
class Transform44TypeClass {};
class EntityTypeClass {};

} // namespace Attr 
//------------------------------------------------------------------------------

