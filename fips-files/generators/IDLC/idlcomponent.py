import IDLC.idltypes as IDLTypes
import genutil as util
import IDLC.idldocument as IDLDocument

# Global component list
components = list()

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
class ComponentDefinition:
    def __init__(self, componentName, comp):
        self.componentName = componentName
        self.variables = list()
        self.isFlag = False
        self.isStruct = False
        self.isManaged = False
        self.isResource = False
        if isinstance(comp, dict):
            if "_managed_" in comp:
                self.isManaged = comp["_managed_"]
            if not "_type_" in comp:
                for varName, var in comp.items():
                    self.variables.append(GetVariableFromEntry(varName, var))
                self.isStruct = True
            else:
                var = GetVariableFromEntry(componentName, comp)
                if var.type is None:
                    self.isFlag = True
                self.variables.append(var)
        else:
            var = GetVariableFromEntry(componentName, comp)
            if var.type is None:
                    self.isFlag = True
            self.variables.append(GetVariableFromEntry(componentName, comp))
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
            util.fmtError("ComponentDefinition does not contain a single variable!")
        elif numVars == 1 and not self.isStruct: # special case: only one variable. This sets the default name of the structs inner variable to "value".
            retVal = 'struct {}\n{{\n'.format(self.componentName)
            if self.variables[0].defaultValue is None:
                retVal += '    {} {};\n'.format(self.variables[0].type, "value")
            else:
                retVal += '    {} {} = {};\n'.format(self.variables[0].type, "value", self.variables[0].defaultValue)
            retVal += "    DECLARE_COMPONENT;\n"
            retVal += "};\n"
            return retVal
        else:
            retVal = 'struct {}\n{{\n'.format(self.componentName)
            for v in self.variables:
                retVal += '    {}\n'.format(v.AsString())
            retVal += "    DECLARE_COMPONENT;\n"
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
def ParseComponents(document):
    if "components" in document:
        for componentName, comp in document["components"].items():
            components.append(ComponentDefinition(componentName, comp))

#------------------------------------------------------------------------------
##
#
def ContainsResourceTypes():
    for comp in components:
        if comp.isResource:
            return True
    return False

#------------------------------------------------------------------------------
##
#
def ContainsEntityTypes():
    for comp in components:
        for var in comp.variables:
            if var.type == IDLTypes.GetTypeString("entity"):
                return True
    return False

#------------------------------------------------------------------------------
##
#
def WriteComponentForwardDeclarations(f, document):
    f.WriteLine("namespace MemDb { class TypeRegistry; }")

#------------------------------------------------------------------------------
##
#
def WriteComponentHeaderDeclarations(f, document):
    for p in components:
        if not p.isFlag:
            f.WriteLine(p.AsTypeDefString())

#------------------------------------------------------------------------------
##
#
def WriteComponentHeaderDetails(f, document):
    f.WriteLine('extern const bool {}_registered;'.format(f.fileName))
    pass

#------------------------------------------------------------------------------
##
#
def HasStructComponents():
    for comp in components:
        if comp.isStruct:
            return True
    return False

#------------------------------------------------------------------------------
##
#
def WriteComponentSourceDefinitions(f, document):
    for comp in components:
        if comp.isFlag is False:
            f.WriteLine('DEFINE_COMPONENT({});'.format(comp.componentName))
    IDLDocument.BeginNamespaceOverride(f, document, "Details")
    f.WriteLine('const bool RegisterComponentLibrary_{filename}()'.format(filename=f.fileName))
    f.WriteLine('{')
    f.IncreaseIndent()
    f.WriteLine('// Make sure string atom tables have been set up.')
    f.WriteLine('Core::SysFunc::Setup();')
    for comp in components:
        defval = comp.componentName + "()"
        #if not comp.isStruct :
        #    if comp.variables[0].defaultValue is not None:
        #        defval = comp.variables[0].defaultValue
        f.WriteLine('{')
        f.WriteLine('Util::StringAtom const name = "{}"_atm;'.format(comp.componentName))
        if comp.isFlag is False:
            flags = 0 if not comp.isManaged else 1 # this must be equal to managed component flag in Game::ComponentFlags
            f.WriteLine('MemDb::ComponentId const cid = MemDb::TypeRegistry::Register<{type}>(name, {defval}, {flags});'.format(type=comp.componentName, defval=defval, flags=flags))
            if comp.isStruct:
                f.WriteLine('Game::ComponentSerialization::Register<{type}>(cid);'.format(type=comp.componentName))
                f.WriteLine('Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<{type}>);'.format(type=comp.componentName))
            else:
                f.WriteLine('Game::ComponentSerialization::Register<{type}>(cid);'.format(type=comp.variables[0].type))
                f.WriteLine('Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<{type}>);'.format(type=comp.variables[0].type))
        else:
            f.WriteLine('MemDb::TypeRegistry::Register(name, 0, nullptr);')
        f.WriteLine('}')
    f.WriteLine("return true;")
    f.DecreaseIndent()
    f.WriteLine("}")
    f.WriteLine('const bool {filename}_registered = RegisterComponentLibrary_{filename}();'.format(filename=f.fileName))
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
    for comp in components:
        if not comp.isStruct:
            continue

        f.WriteLine('template<> void JsonReader::Get<{namespace}::{name}>({namespace}::{name}& ret, const char* attr)'.format(namespace=namespace, name=comp.componentName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine('ret = {namespace}::{name}();'.format(namespace=namespace, name=comp.componentName))
        f.WriteLine("const pjson::value_variant* node = this->GetChild(attr);")
        f.WriteLine("if (node->is_object())")
        f.WriteLine("{")
        f.IncreaseIndent()
        f.WriteLine("this->SetToNode(attr);")
        for var in comp.variables:
            f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get<{type}>(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=var.type))
        f.WriteLine("this->SetToParent();")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

        f.WriteLine('template<> void JsonWriter::Add<{namespace}::{name}>({namespace}::{name} const& value, Util::String const& attr)'.format(namespace=namespace, name=comp.componentName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine("this->BeginObject(attr.AsCharPtr());")
        for var in comp.variables:
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
