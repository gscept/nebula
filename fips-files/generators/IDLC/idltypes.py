import genutil as util

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
    elif (T == "entity"):
        return "Game::Entity"
    elif (T == "color"):
        return "Util::Color"
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
        return "Mathf.Vector2Int"
    elif (T == "vec2"):
        return "Mathf.Vector2"
    elif (T == "vec3"):
        return "Mathf.Vector3"
    elif (T == "vec4"):
        return "Mathf.Vector4"
    elif (T == "vector"):
        return "Mathf.Vector3"
    elif (T == "point"):
        return "Mathf.Vector3"
    elif (T == "quat"):
        return "Mathf.Quaternion"
    elif (T == "mat4"):
        return "Mathf.Matrix"
    elif (T == "string"):
        return "string"
    elif (T == "resource"):
        return "ResourceDescriptor"
    elif (T == "entity"):
        return "EntityId"
    elif (T == "color"):
        return "Mathf.Vector4" # XNA Mathf.Color is a packed uint, while Util::Color is a vec4. We need to reflect the native type in C#
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
    elif (T == "color"):
        return "Util::Color const&"
    elif (T == "entity"):
        return "Game::Entity"
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
    elif (T == "entity"):
        return "Game::Entity::Invalid()"
    elif (T == "color"):
        return "Math::vec4(0,0,0,0)"
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
