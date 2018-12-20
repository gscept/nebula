import genutil as util


#------------------------------------------------------------------------------
## Constants
#
PACKED_PER_ATTRIBUTE = 0
PACKED_PER_INSTANCE = 1

#------------------------------------------------------------------------------
##
#
def GetEventEnum(string):
    s = string.lower()
    if s == "beginframe":
        return "Game::ComponentEvent::OnBeginFrame"
    elif s == "render":
        return "Game::ComponentEvent::OnRender"
    elif s == "endframe":
        return "Game::ComponentEvent::OnEndFrame"
    elif s == "renderdebug":
        return "Game::ComponentEvent::OnRenderDebug"
    elif s == "onactivate":
        return "Game::ComponentEvent::OnActivate"
    elif s == "ondeactivate":
        return "Game::ComponentEvent::OnDeactivate"
    elif s == "onload":
        return "Game::ComponentEvent::OnLoad"
    elif s == "onsave":
        return "Game::ComponentEvent::OnSave"
    else:
        util.fmtError('"{}" is not a valid event!'.format(string))

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
    elif (T == "double"):
        return "Double"
    elif (T == "bool"):
        return "Bool"
    elif (T == "float2"):
        return "Float2"
    elif (T == "float4"):
        return "Float4"
    elif (T == "vector"):
        return "Vector"
    elif (T == "point"):
        return "Point"
    elif (T == "quaternion"):
        return "Quaternion"
    elif (T == "matrix44"):
        return "Matrix44"
    elif (T == "string"):
        return "String"
    elif (T == "resource"):
        return "String"
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
    elif (T == "int64"):
        return "int64_t"
    elif (T == "uint64"):
        return "uint64_t"
    elif (T == "double"):
        return "double"
    elif (T == "bool"):
        return "bool"
    elif (T == "float2"):
        return "Math::float2"
    elif (T == "float4"):
        return "Math::float4"
    elif (T == "vector"):
        return "Math::vector"
    elif (T == "point"):
        return "Math::point"
    elif (T == "quaternion"):
        return "Math::quaternion"
    elif (T == "matrix44"):
        return "Math::matrix44"
    elif (T == "string"):
        return "Util::String"
    elif (T == "resource"):
        return "Util::String"
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
    elif (T == "int64"):
        return "int64_t"
    elif (T == "uint64"):
        return "uint64_t"
    elif (T == "double"):
        return "double"
    elif (T == "bool"):
        return "bool"
    elif (T == "float2"):
        return "Math::float2 const&"
    elif (T == "float4"):
        return "Math::float4 const&"
    elif (T == "vector"):
        return "Math::vector const&"
    elif (T == "point"):
        return "Math::point const&"
    elif (T == "quaternion"):
        return "Math::quaternion const&"
    elif (T == "matrix44"):
        return "Math::matrix44 const&"
    elif (T == "string"):
        return "Util::String const&"
    elif (T == "resource"):
        return "Util::String const&"
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
    else:
        return attrType

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
            # recursion mother fucker
            string += DefaultToString(number)
        return string
    elif type(default) is bool:
        return str(default).lower()
    elif type(default) is str:
        return '"{}"'.format(default)

#------------------------------------------------------------------------------
##
#
def AccessModeToClassString(accessMode):
    access = accessMode.lower()
    if (access == "rw"):
        return "Attr::ReadWrite"
    elif (access == "r"):
        return "Attr::ReadOnly"
    else:
        util.fmtError('"{}" is not a valid access mode!'.format(access))
