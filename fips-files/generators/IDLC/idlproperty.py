import IDLC.idltypes as IDLTypes
import genutil as util
import IDLC.idldocument as IDLDocument

# Global property list
properties = list()

#------------------------------------------------------------------------------
##
#
class VariableDefinition:
    def __init__(self, T, name, defVal):
        if not isinstance(T, str) and T is not None:
            util.fmtError('_type_ value is not a string value!')
        self.type = T
        if not isinstance(name, str):
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
class PropertyDefinition:
    def __init__(self, propertyName, prop):
        self.propertyName = propertyName
        self.variables = list()
        self.isFlag = False
        self.isStruct = False
        self.isResource = False
        if isinstance(prop, dict):
            if not "_type_" in prop:
                for varName, var in prop.items():
                    self.variables.append(GetVariableFromEntry(varName, var))
                self.isStruct = True
            else:
                var = GetVariableFromEntry(propertyName, prop)
                if var.type is None:
                    self.isFlag = True
                self.variables.append(var)
        else:
            var = GetVariableFromEntry(propertyName, prop)
            if var.type is None:
                    self.isFlag = True
            self.variables.append(GetVariableFromEntry(propertyName, prop))
        # Check to see if any of the types within the struct are resource.
        for var in self.variables:
            if var.type == IDLTypes.GetTypeString("resource"):
                if self.isStruct:
                    util.fmtError("Structs containing resources not supported!")
                self.isResource = True

    def AsTypeDefString(self):
        if self.isFlag:
            return ""
        numVars = len(self.variables)
        if numVars == 0:
            util.fmtError("PropertyDefinition does not contain a single variable!")
        elif numVars == 1 and not self.isStruct: # special case: only one variable. This sets the default name of the structs inner variable to "value".
            retVal = 'struct {}\n{{\n'.format(self.propertyName)
            if self.variables[0].defaultValue is None:
                retVal += '    {} {};\n'.format(self.variables[0].type, "value")
            else:
                retVal += '    {} {} = {};\n'.format(self.variables[0].type, "value", self.variables[0].defaultValue)
            retVal += "    DECLARE_PROPERTY;\n"
            retVal += "};\n"
            return retVal
        else:
            retVal = 'struct {}\n{{\n'.format(self.propertyName)
            for v in self.variables:
                retVal += '    {}\n'.format(v.AsString())
            retVal += "    DECLARE_PROPERTY;\n"
            retVal += "};\n"
            return retVal
    pass

#------------------------------------------------------------------------------
##
#
def Capitalize(s):
    return s[:1].upper() + s[1:]

#------------------------------------------------------------------------------
##
#
def GetVariableFromEntry(name, var):
    if isinstance(var, dict):
        if not "_type_" in var:
            util.fmtError('{} : {}'.format(name, var.__repr__()))
        
        default = None
        
        if var["_type_"] == "_flag_":
            return VariableDefinition(None, name, None)
        
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
def ParseProperties(document):
    if "properties" in document:
        for propertyName, prop in document["properties"].items():
            properties.append(PropertyDefinition(propertyName, prop))

#------------------------------------------------------------------------------
##
#
def ContainsResourceTypes():
    for prop in properties:
        if prop.isResource:
            return True
    return False

#------------------------------------------------------------------------------
##
#
def ContainsEntityTypes():
    for prop in properties:
        for var in prop.variables:
            if var.type == IDLTypes.GetTypeString("entity"):
                return True
    return False

#------------------------------------------------------------------------------
##
#
def WritePropertyForwardDeclarations(f, document):
    f.WriteLine("namespace MemDb { class TypeRegistry; }")

#------------------------------------------------------------------------------
##
#
def WritePropertyHeaderDeclarations(f, document):
    for p in properties:
        if not p.isFlag:
            f.WriteLine(p.AsTypeDefString())

#------------------------------------------------------------------------------
##
#
def WritePropertyHeaderDetails(f, document):
    f.WriteLine('extern const bool {}_registered;'.format(f.fileName))
    pass

#------------------------------------------------------------------------------
##
#
def HasStructProperties():
    for prop in properties:
        if prop.isStruct:
            return True
    return False

#------------------------------------------------------------------------------
##
#
def WritePropertySourceDefinitions(f, document):
    for prop in properties:
        if prop.isFlag is False:
            f.WriteLine('DEFINE_PROPERTY({});'.format(prop.propertyName))
    IDLDocument.BeginNamespaceOverride(f, document, "Details")
    f.WriteLine('const bool RegisterPropertyLibrary_{filename}()'.format(filename=f.fileName))
    f.WriteLine('{')
    f.IncreaseIndent()
    f.WriteLine('// Make sure string atom tables have been set up.')
    f.WriteLine('Core::SysFunc::Setup();')
    for prop in properties:
        defval = prop.propertyName + "()"
        #if not prop.isStruct :
        #    if prop.variables[0].defaultValue is not None:
        #        defval = prop.variables[0].defaultValue
        f.WriteLine('{')
        f.WriteLine('Util::StringAtom const name = "{}"_atm;'.format(prop.propertyName))
        if prop.isFlag is False:
            f.WriteLine('MemDb::PropertyId const pid = MemDb::TypeRegistry::Register<{type}>(name, {defval});'.format(type=prop.propertyName, defval=defval))
            if prop.isStruct:
                f.WriteLine('Game::PropertySerialization::Register<{type}>(pid);'.format(type=prop.propertyName))
            else:
                f.WriteLine('Game::PropertySerialization::Register<{type}>(pid);'.format(type=prop.variables[0].type))
            f.WriteLine('Game::PropertyInspection::Register(pid, &Game::PropertyDrawFuncT<{type}>);'.format(type=prop.propertyName))
        else:
            f.WriteLine('MemDb::TypeRegistry::Register(name, 0, nullptr);')
        f.WriteLine('}')
    f.WriteLine("return true;")
    f.DecreaseIndent()
    f.WriteLine("}")
    f.WriteLine('const bool {filename}_registered = RegisterPropertyLibrary_{filename}();'.format(filename=f.fileName))
    IDLDocument.EndNamespaceOverride(f, document, "Details")

#------------------------------------------------------------------------------
##
#
def WriteEnumJsonSerializers(f, document):
    namespace = IDLDocument.GetNamespace(document)
    for enumName, enum in document["enums"].items():
        f.WriteLine('template<> void JsonReader::Get<{namespace}::{name}>({namespace}::{name}& ret, const char* attr)'.format(namespace=namespace, name=enumName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine("const pjson::value_variant* node = this->GetChild(attr);")
        f.WriteLine("if (node->is_string())")
        f.WriteLine("{")
        f.IncreaseIndent()
        f.WriteLine("Util::String str = node->as_string_ptr();")
        for value in enum:
            f.WriteLine('if (str == "{val}") {{ ret = {namespace}::{name}::{val}; return; }}'.format(val=value, namespace=namespace, name=enumName))
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("else if (node->is_int())")
        f.WriteLine("{")
        f.IncreaseIndent()
        f.WriteLine('ret = ({namespace}::{name})node->as_int32();'.format(namespace=namespace, name=enumName))
        f.WriteLine('return;')
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine('ret = {namespace}::{name}();'.format(namespace=namespace, name=enumName))
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

        f.WriteLine('template<> void JsonWriter::Add<{namespace}::{name}>({namespace}::{name} const& value, Util::String const& attr)'.format(namespace=namespace, name=enumName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine('this->Add<int>(value, attr);');
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

#------------------------------------------------------------------------------
##
#
def WriteStructJsonSerializers(f, document):
    namespace = IDLDocument.GetNamespace(document)
    for prop in properties:
        if not prop.isStruct:
            continue

        f.WriteLine('template<> void JsonReader::Get<{namespace}::{name}>({namespace}::{name}& ret, const char* attr)'.format(namespace=namespace, name=prop.propertyName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine('ret = {namespace}::{name}();'.format(namespace=namespace, name=prop.propertyName))
        f.WriteLine("const pjson::value_variant* node = this->GetChild(attr);")
        f.WriteLine("if (node->is_object())")
        f.WriteLine("{")
        f.IncreaseIndent()
        for var in prop.variables:
            f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get<{type}>(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=var.type))
        f.DecreaseIndent()
        f.WriteLine("}")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

        f.WriteLine('template<> void JsonWriter::Add<{namespace}::{name}>({namespace}::{name} const& value, Util::String const& attr)'.format(namespace=namespace, name=prop.propertyName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine("this->BeginObject(attr.AsCharPtr());")
        for var in prop.variables:
            f.WriteLine('this->Add<{type}>(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=var.type))
        f.WriteLine("this->End();")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")


#------------------------------------------------------------------------------
##
#
def WriteEnumeratedTypes(f, document):
    if "enums" in document:
        for enumName, enum in document["enums"].items():
            # Declare Enums
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
