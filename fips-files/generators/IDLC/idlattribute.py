import IDLC.idltypes as IDLTypes
import genutil as util

# fight me
attributes = list()

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
class VariableDefinition:
    def __init__(self, T, name, defVal):
        if not isinstance(T, str):
            util.fmtError('_type_ value is not a string value!')
        self.type = T
        if not isinstance(T, str):
            util.fmtError('_name_ value is not a string value!')
        self.name = name
        self.defaultValue = defVal
    
    def AsString(self):
        if self.defaultValue is None:
            return '{} {};'.format(self.type, self.name)
        else:
            return '{} {} = {};'.format(self.type, self.name, self.defaultValue)
    pass

#------------------------------------------------------------------------------
##
#
class AttributeDefinition:
    def __init__(self, attributeName, attribute):
        self.attributeName = attributeName
        self.variables = list()
        self.isStruct = False
        if isinstance(attribute, dict):
            if not "_type_" in attribute:
                for varName, var in attribute.items():
                    self.variables.append(GetVariableFromEntry(varName, var))
            else:
                self.variables.append(GetVariableFromEntry(attributeName, attribute))
        else:
            self.variables.append(GetVariableFromEntry(attributeName, attribute))
        if len(self.variables) > 1:
            self.isStruct = True

    def AsTypeDefString(self):
        numVars = len(self.variables)
        if numVars == 0:
            util.fmtError("AttributeDefinition does not contain a single variable!")
        elif numVars == 1:
            return 'typedef {} {};\n'.format(self.variables[0].type, self.variables[0].name)
        else:
            varDefs = ""
            retVal = 'struct {}\n{{\n'.format(self.attributeName)
            for v in self.variables:
                retVal += '    {}\n'.format(v.AsString())
            retVal += "};\n"
            return retVal
    pass

#------------------------------------------------------------------------------
##
#
def GetVariableFromEntry(name, var):
    if isinstance(var, dict):
        if not "_type_" in var:
            util.fmtError('{} : {}'.format(name, var.__repr__()))
        
        default = None
        
        
        if "_default_" in var:
            default = IDLTypes.GetTypeString(var["_type_"]) + "(" + IDLTypes.DefaultToString(var["_default_"]) + ")"
        else:
            default = IDLTypes.DefaultValue(var["_type_"])

        return VariableDefinition(IDLTypes.GetTypeString(var["_type_"]), name, default)
    else:
        return VariableDefinition(IDLTypes.GetTypeString(var), name, IDLTypes.DefaultValue(var))

#------------------------------------------------------------------------------
##
#
def WriteAttributeHeaderDeclarations(f, document):
    for attributeName, attribute in document["properties"].items():
        attributes.append(AttributeDefinition(attributeName, attribute))
    for attr in attributes:
        f.WriteLine(attr.AsTypeDefString())

#------------------------------------------------------------------------------
##
#
def WriteAttributeHeaderDetails(f, document):
    f.WriteLine('inline const bool RegisterAttributeLibrary_{filename}()'.format(filename=f.fileName))
    f.WriteLine('{')
    f.IncreaseIndent()
    f.WriteLine('// Make sure string atom tables have been set up.')
    f.WriteLine('Core::SysFunc::Setup();')
    for attr in attributes:
        defval = attr.attributeName + "()"
        if not attr.isStruct :
            if attr.variables[0].defaultValue is not None:
                defval = attr.variables[0].defaultValue
        f.WriteLine('MemDb::TypeRegistry::Register<{type}>("{type}"_atm, {defval});'.format(type=attr.attributeName, defval=defval))
    f.WriteLine("return true;")
    f.DecreaseIndent()
    f.WriteLine("}")
    f.WriteLine('static const bool {filename}_registered = RegisterAttributeLibrary_{filename}();'.format(filename=f.fileName))

#------------------------------------------------------------------------------
##
#
def WriteAttributeSourceDefinitions(f, document):
    f.WriteLine("")

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