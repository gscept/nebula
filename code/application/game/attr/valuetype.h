#pragma once
//------------------------------------------------------------------------------
/**
    @file attr/valuetype.h

    Defines the valid attribute value types as enum.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file

	@todo	Add quaternions and additional types that are missing
*/
#include "core/types.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace Attr
{
enum ValueType
{
	VoidType = Util::Variant::Void, // This is for non-value attributes.
	ByteType = Util::Variant::Byte,
	ShortType = Util::Variant::Short,
	UShortType = Util::Variant::UShort,
	IntType = Util::Variant::Int,
	UIntType = Util::Variant::UInt,
	Int64Type = Util::Variant::Int64,
	UInt64Type = Util::Variant::UInt64,
	FloatType = Util::Variant::Float,
	DoubleType = Util::Variant::Double,
	BoolType = Util::Variant::Bool,
	Float2Type = Util::Variant::Float2,
	Float4Type = Util::Variant::Float4,
	QuaternionType = Util::Variant::Quaternion,
	StringType = Util::Variant::String,
	Matrix44Type = Util::Variant::Matrix44,
	Transform44Type = Util::Variant::Transform44,
	BlobType = Util::Variant::Blob,
	GuidType = Util::Variant::Guid,
	VoidPtrType = Util::Variant::VoidPtr,
	IntArrayType = Util::Variant::IntArray,
	FloatArrayType = Util::Variant::FloatArray,
	BoolArrayType = Util::Variant::BoolArray,
	Float2ArrayType = Util::Variant::Float2Array,
	Float4ArrayType = Util::Variant::Float4Array,
	StringArrayType = Util::Variant::StringArray,
	Matrix44ArrayType = Util::Variant::Matrix44Array,
	BlobArrayType = Util::Variant::BlobArray,
	GuidArrayType = Util::Variant::GuidArray,
	EntityType = Util::Variant::NumTypes + 1,
};

// these class definitions are just for template specializations later on,
// templates are also possible with enums, but they don't look very
// intuitive in the debugger...
class ByteTypeClass {};
class ShortTypeClass {};
class UShortTypeClass {};
class IntTypeClass {};
class UIntTypeClass {};
class Int64TypeClass {};
class UInt64TypeClass {};
class FloatTypeClass {};
class DoubleTypeClass {};
class BoolTypeClass {};
class Float2TypeClass {};
class Float4TypeClass {};
class QuaternionTypeClass {};
class StringTypeClass {};
class Matrix44TypeClass {};
class Transform44TypeClass {};
class BlobTypeClass {};
class GuidTypeClass {};
class VoidPtrTypeClass {};
class IntArrayTypeClass {};
class FloatArrayTypeClass {};
class BoolArrayTypeClass {};
class Float2ArrayTypeClass {};
class Float4ArrayTypeClass {};
class StringArrayTypeClass {};
class Matrix44ArrayTypeClass {};
class BlobArrayTypeClass {};
class GuidArrayTypeClass {};
class EntityTypeClass {};

} // namespace Attr
//------------------------------------------------------------------------------

