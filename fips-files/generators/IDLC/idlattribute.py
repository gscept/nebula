import IDLC.idltypes as IDLTypes
import genutil as util

def Capitalize(s):
    return s[:1].upper() + s[1:]

def GetTypeCamelNotation(attributeName, attribute, document):
    if not "type" in attribute:
        util.fmtError('Attribute type is required. Attribute "{}" does not name a type!'.format(attributeName))
    typeString = IDLTypes.ConvertToCamelNotation(attribute["type"])

    if not typeString:
        # Figure out what type it actually is.
        if attribute["type"] in document["enums"]:
            typeString = IDLTypes.ConvertToCamelNotation("uint") # type for enums is uint
        else:
            util.fmtError('"{}" is not a valid type!'.format(attribute["type"]))
    return typeString

#------------------------------------------------------------------------------
##
#
def WriteAttributeHeaderDeclarations(f, document):
    for attributeName, attribute in document["attributes"].items():
        typeString = IDLTypes.GetTypeString(attribute["type"])

        if not "fourcc" in attribute:
            util.fmtError('Attribute FourCC is required. Attribute "{}" does not have a fourcc!'.format(attributeName))
        fourcc = attribute["fourcc"]

        accessMode = "rw"
        if "access" in attribute:
            accessMode = IDLTypes.AccessModeToClassString(attribute["access"])
        
        defVal = IDLTypes.DefaultValue(attribute["type"])
        if "default" in attribute:
            default = IDLTypes.DefaultToString(attribute["default"])
            defVal = "{}({})".format(IDLTypes.GetTypeString(attribute["type"]), default)

        f.WriteLine('__DeclareAttribute({}, {}, \'{}\', {}, {});'.format(Capitalize(attributeName), typeString, fourcc, accessMode, defVal))

#------------------------------------------------------------------------------
##
#
def WriteEnumeratedTypes(f, document):
    if "enums" in document:
        for enumName, enum in document["enums"].items():
            # Declare Enums
            f.InsertNebulaDivider()
            f.WriteLine("enum {}".format(enumName))
            f.WriteLine("{")
            f.IncreaseIndent()
            numValues = 0
            for key, value in enum.items():
                f.WriteLine("{} = {},".format(key, value))
                numValues += 1
            f.WriteLine("Num{} = {}".format(Capitalize(enumName), numValues))
            f.DecreaseIndent()
            f.WriteLine("};")
            f.WriteLine("")