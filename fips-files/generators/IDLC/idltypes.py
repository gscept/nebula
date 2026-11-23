import genutil as util

#------------------------------------------------------------------------------
##
#
def IsStdArrayType(attrType):
    T = attrType.replace(" ", "")
    return T.startswith("std::array<") and T.endswith(">")

#------------------------------------------------------------------------------
##
#
def GetCppTypeString(attrType):
    T = attrType.lower()
    if (T == "byte"):
        return "byte"
    elif (T == "short"):
        return "short"
    elif (T == "ushort"):
        return "ushort"
    elif (T == "int"):
        return "int"
    elif (T == "uint"):
        return "uint"
    elif (T == "int32"):
        return "int32_t"
    elif (T == "uint32"):
        return "uint32_t"
    elif (T == "uint16"):
        return "uint16_t"
    elif (T == "int16"):
        return "int16_t"
    elif (T == "float"):
        return "float"
    elif (T == "scalar"):
        return "Math::scalar"
    elif (T == "int64"):
        return "int64_t"
    elif (T == "uint64"):
        return "uint64_t"
    elif (T == "double"):
        return "double"
    elif (T == "bool"):
        return "bool"
    elif (T == "int2"):
        return "Math::int2"
    elif (T == "vec2"):
        return "Math::vec2"
    elif (T == "vec3"):
        return "Math::vec3"
    elif (T == "vec4"):
        return "Math::vec4"
    elif (T == "vector"):
        return "Math::vector"
    elif (T == "point"):
        return "Math::point"
    elif (T == "quat"):
        return "Math::quat"
    elif (T == "mat4"):
        return "Math::mat4"
    elif (T == "string"):
        return "Util::String"
    elif (T == "util::stringatom"):
        return "Util::StringAtom"
    elif (T == "resource"):
        return "Resources::ResourceName"
    elif (T == "entity"):
        return "Game::Entity"
    elif (T == "color"):
        return "Util::Color"
    elif (T.startswith("[") and T.endswith("]")):
        return "Util::Array<{}>".format(GetCppTypeString(attrType[1:-1]))
    elif (T == "bitfield16"):
        return "Util::BitField<16>"
    elif (T == "bitfield32"):
        return "Util::BitField<32>"
    elif (T == "bitfield64"):
        return "Util::BitField<64>"
    else:
        return attrType
    
#------------------------------------------------------------------------------
##
#
def GetJsonParserTypeString(attrType):
    T = attrType.lower()
    if (T == "byte"):
        return "byte"
    elif (T == "short"):
        return "short"
    elif (T == "ushort"):
        return "ushort"
    elif (T == "int"):
        return "int"
    elif (T == "uint"):
        return "uint"
    elif (T == "int32"):
        return "int32_t"
    elif (T == "uint32"):
        return "uint32_t"
    elif (T == "uint16"):
        return "uint16_t"
    elif (T == "int16"):
        return "int16_t"
    elif (T == "float"):
        return "float"
    elif (T == "scalar"):
        return "Math::scalar"
    elif (T == "int64"):
        return "int64_t"
    elif (T == "uint64"):
        return "uint64_t"
    elif (T == "double"):
        return "double"
    elif (T == "bool"):
        return "bool"
    elif (T == "int2"):
        return "Math::int2"
    elif (T == "vec2"):
        return "Math::vec2"
    elif (T == "vec3"):
        return "Math::vec3"
    elif (T == "vec4"):
        return "Math::vec4"
    elif (T == "vector"):
        return "Math::vector"
    elif (T == "point"):
        return "Math::point"
    elif (T == "quat"):
        return "Math::quat"
    elif (T == "mat4"):
        return "Math::mat4"
    elif (T == "string"):
        return "Util::String"
    elif (T == "util::stringatom"):
        return "Util::StringAtom"
    elif (T == "resource"):
        return "Resources::ResourceName"
    elif (T == "entity"):
        return "Game::Entity"
    elif (T == "color"):
        return "Util::Color"
    elif (T.startswith("[") and T.endswith("]")):
        return "Util::Array<{}>".format(GetJsonParserTypeString(attrType[1:-1]))
    elif (T == "bitfield16"):
        return "16"
    elif (T == "bitfield32"):
        return "32"
    elif (T == "bitfield64"):
        return "64"
    else:
        return attrType


#------------------------------------------------------------------------------
##
#
def GetCsTypeString(attrType):
    T = attrType.lower()
    if (T == "byte"):
        return "byte"
    elif (T == "short"):
        return "short"
    elif (T == "ushort"):
        return "ushort"
    elif (T == "int"):
        return "int"
    elif (T == "uint"):
        return "uint"
    elif (T == "int32"):
        return "int"
    elif (T == "uint32"):
        return "uint"
    elif (T == "int16"):
        return "short"
    elif (T == "uint16"):
        return "ushort"
    elif (T == "uint16_t"):
        return "ushort"
    elif (T == "float"):
        return "float"
    elif (T == "scalar"):
        return "float"
    elif (T == "int64"):
        return "Int64"
    elif (T == "uint64"):
        return "UInt64"
    elif (T == "double"):
        return "double"
    elif (T == "bool"):
        return "bool"
    elif (T == "int2"):
        return "Vector2Int"
    elif (T == "vec2"):
        return "Vector2"
    elif (T == "vec3"):
        return "Vector3"
    elif (T == "vec4"):
        return "Vector4"
    elif (T == "vector"):
        return "Vector3"
    elif (T == "point"):
        return "Vector3"
    elif (T == "quat"):
        return "Quaternion"
    elif (T == "mat4"):
        return "Matrix"
    elif (T == "string"):
        return "string"
    elif (T == "util::stringatom"):
        return "ResourceDescriptor"    # Util::StringAtom is a pointer, ResourceDescriptor is its blittable mirror
    elif (T == "resource"):
        return "ResourceDescriptor"
    elif (T == "entity"):
        return "EntityId"
    elif (T == "color"):
        return "Vector4" # XNA Mathf.Color is a packed uint, while Util::Color is a vec4. We need to reflect the native type in C#
    elif (T == "bitfield16"):
        return "ushort"
    elif (T == "bitfield32"):
        return "uint"
    elif (T == "bitfield64"):
        return "UInt64"
    elif (T == "physics::characterid"):
        return "uint"
    elif (T.startswith("[") and T.endswith("]")):
        return "[{}]".format(GetCppTypeString(attrType[1:-1]))
    else:
        return attrType

#------------------------------------------------------------------------------
##
#   Return the typestring as an argument type.
#   This suffixes the type with const& if type is larger than 64 bits.
#   Returns the original attrtype argument if none is found.
#
def GetArgumentType(attrType):
    T = attrType.lower()
    if (T == "byte"):
        return "byte"
    elif (T == "short"):
        return "short"
    elif (T == "ushort"):
        return "ushort"
    elif (T == "int"):
        return "int"
    elif (T == "uint"):
        return "uint"
    elif (T == "int32"):
        return "int32_t"
    elif (T == "uint32"):
        return "uint32_t"
    elif (T == "uint16"):
        return "uint16_t"
    elif (T == "int16"):
        return "int16_t"
    elif (T == "float"):
        return "float"
    elif (T == "scalar"):
        return "Math::scalar"
    elif (T == "int64"):
        return "int64_t"
    elif (T == "uint64"):
        return "uint64_t"
    elif (T == "double"):
        return "double"
    elif (T == "bool"):
        return "bool"
    elif (T == "int2"):
        return "Math::int2"
    elif (T == "vec2"):
        return "Math::vec2 const&"
    elif (T == "vec3"):
        return "Math::vec3 const&"
    elif (T == "vec4"):
        return "Math::vec4 const&"
    elif (T == "vector"):
        return "Math::vector const&"
    elif (T == "point"):
        return "Math::point const&"
    elif (T == "quat"):
        return "Math::quat const&"
    elif (T == "mat4"):
        return "Math::mat4 const&"
    elif (T == "string"):
        return "Util::String const&"
    elif (T == "resource"):
        return "Resources::ResourceName"
    elif (T == "color"):
        return "Util::Color const&"
    elif (T == "entity"):
        return "Game::Entity"
    elif (T.startswith("[") and T.endswith("]")):
        return "Util::Array<{}>".format(GetCppTypeString(attrType[1:-1]))
    elif (T == "bitfield16"):
        return "Util::BitField<16> const&"
    elif (T == "bitfield32"):
        return "Util::BitField<32> const&"
    elif (T == "bitfield64"):
        return "Util::BitField<64> const&"
    else:
        return attrType

#------------------------------------------------------------------------------
##
#
def DefaultValue(attrType):
    T = attrType.lower()
    if (T == "byte"):
        return "byte(0)"
    elif (T == "short"):
        return "short(0)"
    elif (T == "ushort"):
        return "ushort(0)"
    elif (T == "int"):
        return "int(0)"
    elif (T == "int32"):
        return "int32_t(0)"
    elif (T == "uint"):
        return "uint32_t(0)"
    elif (T == "uint32"):
        return "uint32_t(0)"
    elif (T == "uint16"):
        return "uint16_t(0)"
    elif (T == "int16"):
        return "int16_t(0)"
    elif (T == "float"):
        return "float(0)"
    elif (T == "scalar"):
        return "Math::scalar(0)"
    elif (T == "int64"):
        return "int64_t(0)"
    elif (T == "uint64"):
        return "uint64_t(0)"
    elif (T == "double"):
        return "double(0.0)"
    elif (T == "bool"):
        return "bool(false)"
    elif (T == "int2"):
        return 'Math::int2{0, 0}'
    elif (T == "vec2"):
        return "Math::vec2(0, 0)"
    elif (T == "vec3"):
        return "Math::vec3(0, 0, 0)"
    elif (T == "vec4"):
        return "Math::vec4(0, 0, 0, 0)"
    elif (T == "vector"):
        return "Math::vector(0, 0, 0)"
    elif (T == "point"):
        return "Math::point(0, 0, 0)"
    elif (T == "quat"):
        return "Math::quat()"
    elif (T == "mat4" or T == "Math::mat4"):
        return "Math::mat4()"
    elif (T == "string"):
        return "Util::String()"
    elif (T == "util::stringatom"):
        return "Util::StringAtom()"
    elif (T == "resource"):
        return "Resources::ResourceName()"
    elif (T == "entity"):
        return "Game::Entity::Invalid()"
    elif (T == "color"):
        return "Math::vec4(0,0,0,0)"
    elif (T == "bitfield16"):
        return "Util::BitField<16>()"
    elif (T == "bitfield32"):
        return "Util::BitField<32>()"
    elif (T == "bitfield64"):
        return "Util::BitField<64>()"
    else:
        return None

#------------------------------------------------------------------------------
##
#
def DefaultToString(default):
    if type(default) is int:
        return str(default)
    elif type(default) is float:
        # todo: if we've got a double type, we don't need to add f specifier
        return "{}f".format(default)
    elif type(default) is list:
        string = ""
        for number in default:
            if len(string) != 0:
                string += ", "
            # recursion
            string += DefaultToString(number)
        return string
    elif type(default) is bool:
        return str(default).lower()
    elif type(default) is str:
        return '"{}"'.format(default)

#------------------------------------------------------------------------------
##
#   Native size and alignment per IDL type, measured from the engine headers.
#   The managed mirror has to reproduce these exactly, because the component api
#   copies the whole struct through Marshal, and Marshal cannot align a field to
#   16 the way Math::mat4 and the SIMD vector types do.
#
NativeLayout = {
    "byte": (1, 1),
    "short": (2, 2),
    "ushort": (2, 2),
    "int16": (2, 2),
    "int16_t": (2, 2),
    "uint16": (2, 2),
    "uint16_t": (2, 2),
    "int": (4, 4),
    "uint": (4, 4),
    "int32": (4, 4),
    "int32_t": (4, 4),
    "uint32": (4, 4),
    "uint32_t": (4, 4),
    "float": (4, 4),
    "scalar": (4, 4),
    "math::scalar": (4, 4),
    "int64": (8, 8),
    "int64_t": (8, 8),
    "uint64": (8, 8),
    "uint64_t": (8, 8),
    "double": (8, 8),
    "bool": (1, 1),
    "int2": (8, 4),
    "math::int2": (8, 4),
    "vec2": (8, 4),
    "math::vec2": (8, 4),
    "vec3": (16, 16),
    "math::vec3": (16, 16),
    "vec4": (16, 16),
    "math::vec4": (16, 16),
    "vector": (16, 16),
    "math::vector": (16, 16),
    "point": (16, 16),
    "math::point": (16, 16),
    "quat": (16, 16),
    "math::quat": (16, 16),
    "mat4": (64, 16),
    "math::mat4": (64, 16),
    "util::stringatom": (8, 8),
    "resource": (8, 8),
    "resources::resourcename": (8, 8),
    "entity": (8, 8),
    "game::entity": (8, 8),
    "physics::characterid": (4, 4),
    "color": (16, 16),
    "util::color": (16, 16),
    "bitfield16": (2, 2),
    "util::bitfield<16>": (2, 2),
    "bitfield32": (4, 4),
    "util::bitfield<32>": (4, 4),
    "bitfield64": (8, 8),
    "util::bitfield<64>": (8, 8),
}

#------------------------------------------------------------------------------
##
#   Return (size, align) for an IDL type, or None when the type has no blittable
#   managed mirror. Util::String is 32 bytes natively and a C# string field is a
#   pointer, and Util::Array is not blittable at all, so neither is in the table.
#
def GetNativeLayout(attrType, document=None):
    T = attrType.lower().replace(" ", "")
    if T in NativeLayout:
        return NativeLayout[T]

    # An enum declared by the document. C# enums and enum class both default to int
    if document is not None and "enums" in document:
        name = T.split("::")[-1]
        for enumName in document["enums"]:
            if enumName.lower() == name:
                return (4, 4)

    return None
