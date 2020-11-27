import genutil as util

#------------------------------------------------------------------------------
##
#
def ConvertToCamelNotation(attrType):
    T = attrType.lower()
    if (T == "byte"):
        return "Byte"
    elif (T == "short"):
        return "Short"
    elif (T == "ushort"):
        return "UShort"
    elif (T == "int"):
        return "Int"
    elif (T == "uint"):
        return "UInt"
    elif (T == "int64"):
        return "Int64"
    elif (T == "uint64"):
        return "UInt64"
    elif (T == "float"):
        return "Float"
    elif (T == "scalar"):
        return "Scalar"
    elif (T == "double"):
        return "Double"
    elif (T == "bool"):
        return "Bool"
    elif (T == "int2"):
        return "Int2"
    elif (T == "vec2"):
        return "Vec2"
    elif (T == "vec3"):
        return "Vec3"
    elif (T == "vec4"):
        return "Vec4"
    elif (T == "vector"):
        return "Vector"
    elif (T == "point"):
        return "Point"
    elif (T == "quat"):
        return "Quaternion"
    elif (T == "mat4"):
        return "Mat4"
    elif (T == "string"):
        return "String"
    elif (T == "resource"):
        return "ResourceName"
    elif (T == "blob"):
        return "Blob"
    elif (T == "guid"):
        return "Guid"
    elif (T == "void*"):
        return "VoidPtr"
    elif (T == "entity"):
        return "Entity"
    elif (T == "variant"):
        return "Variant"
    elif (T == "fourcc"):
        return "FourCC"
    else:
        return None

#------------------------------------------------------------------------------
##
#
def GetTypeString(attrType):
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
    elif (T == "resource"):
        return "Resources::ResourceName"
    elif (T == "blob"):
        return "Util::Blob"
    elif (T == "guid"):
        return "Util::Guid"
    elif (T == "void*"):
        return "void*"
    elif (T == "entity"):
        return "Game::Entity"
    elif (T == "variant"):
        return "Util::Variant"
    elif (T == "fourcc"):
        return "Util::FourCC"
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
    elif (T == "blob"):
        return "Util::Blob const&"
    elif (T == "guid"):
        return "Util::Guid const&"
    elif (T == "void*"):
        return "void*"
    elif (T == "entity"):
        return "Game::Entity"
    elif (T == "variant"):
        return "Util::Variant const&"
    elif (T == "fourcc"):
        return "Util::FourCC"
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
    elif (T == "uint"):
        return "uint(0)"
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
    elif (T == "resource"):
        return "Resources::ResourceName()"
    elif (T == "blob"):
        return "Util::Blob()"
    elif (T == "guid"):
        return "Util::Guid()"
    elif (T == "void*"):
        return "void*"
    elif (T == "entity"):
        return "Game::Entity(-1)"
    elif (T == "variant"):
        return "Util::Variant()"
    elif (T == "fourcc"):
        return None
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
