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
            util.fmtError('"type" value is not a string value!')
        self.type = T
        if not isinstance(name, str):
            util.fmtError('"name" value is not a string value!')
        self.name = name
        self.defaultValue = defVal

        
    def AsString(self):
        if self.defaultValue is None:
            return '{} {};'.format(IDLTypes.GetCppTypeString(self.type), self.name)
        else:
            return '{} {} = {};'.format(IDLTypes.GetCppTypeString(self.type), self.name, self.defaultValue)
    pass

    def AsCsString(self):
        t = IDLTypes.GetCsTypeString(self.type)
        t = t.replace("::", ".")
        return '{} {};'.format(t, self.name)
    pass

#------------------------------------------------------------------------------
##
#
class ComponentDefinition:
    def __init__(self, componentName, comp):
        self.componentName = componentName
        self.variables = list()
        self.hasResource = False
        
        if isinstance(comp, dict):
            for varName, var in comp.items():
                if varName != "_managed_":
                    self.variables.append(GetVariableFromEntry(varName, var))
        else:
            util.fmtError('Invalid component {}!\nComponent definition in NIDL must be JSON dict!'.format(self.componentName))
    pass

    def AsCppTypeDefString(self):
        retVal = 'struct {}\n{{\n'.format(self.componentName)
        for v in self.variables:
            retVal += '    {}\n'.format(v.AsString())
        retVal += "    struct Traits;\n"
        retVal += "};\n"
        return retVal
    pass

    def AsCsTypeDefString(self):
        retVal = 'public struct {} : NativeComponent\n{{\n'.format(self.componentName)
        for v in self.variables:
            retVal += '    public {}\n'.format(v.AsCsString())
        retVal += "}\n"
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
        if not "type" in var:
            util.fmtError('{} : {}'.format(name, var.__repr__()))
        
        default = None
        
        if "default" in var:
            default = IDLTypes.GetCppTypeString(var["type"]) + "(" + IDLTypes.DefaultToString(var["default"]) + ")"
        else:
            default = IDLTypes.DefaultValue(var["type"])

        return VariableDefinition(var["type"], name, default)
    else:
        return VariableDefinition(var, name, IDLTypes.DefaultValue(var))

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
        for var in comp.variables:
            if var.type == "resource":
                return True
    return False

#------------------------------------------------------------------------------
##
#
def ContainsEntityTypes():
    for comp in components:
        for var in comp.variables:
            if var.type == "entity":
                return True
    return False

#------------------------------------------------------------------------------
##
#
def WriteComponentHeaderDeclarations(f, document):
    for c in components:
        f.WriteLine(c.AsCppTypeDefString())
    namespace = IDLDocument.GetNamespace(document)
    for c in components:
        f.InsertNebulaDivider()
        f.WriteLine('struct {}::Traits'.format(c.componentName))
        f.WriteLine("{")
        f.IncreaseIndent()
        f.WriteLine('static_assert(std::is_standard_layout<{}>());'.format(c.componentName))
        f.WriteLine("Traits() = delete;")
        f.WriteLine('using type = {};'.format(c.componentName))
        f.WriteLine('static constexpr auto name = "{}";'.format(c.componentName))
        f.WriteLine('static constexpr auto fully_qualified_name = "{}.{}";'.format(namespace, c.componentName))
        f.WriteLine('static constexpr size_t num_fields = {};'.format(len(c.variables)))
        if (len(c.variables) > 0):
            f.WriteLine('static constexpr const char* field_names[num_fields] = {')
            for v in c.variables:
                f.WriteLine('    "{}",'.format(v.name))
            f.WriteLine('};')
            f.WriteLine('using field_types = std::tuple<')
            for i, v in enumerate(c.variables):
                f.Write('    {}'.format(IDLTypes.GetCppTypeString(v.type)))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('>;')
            f.WriteLine('static constexpr size_t field_byte_offsets[num_fields] = {')
            for v in c.variables:
                f.WriteLine('    offsetof({}, {}),'.format(c.componentName, v.name))
            f.WriteLine('};')
        else:
            f.WriteLine('static constexpr const char** field_names = nullptr;')
            f.WriteLine('static constexpr size_t* field_byte_offsets = nullptr;')


        
        f.DecreaseIndent()
        f.WriteLine("};")

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
        f.WriteLine('this->Add<int>(value, attr);')
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

#------------------------------------------------------------------------------
##
#
def WriteStructJsonSerializers(f, document):
    namespace = IDLDocument.GetNamespace(document)
    for comp in components:
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
            f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get<{type}>(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetCppTypeString(var.type)))
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
            f.WriteLine('this->Add<{type}>(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetCppTypeString(var.type)))
        f.WriteLine("this->End();")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")


#------------------------------------------------------------------------------
##
#
def WriteEnumeratedCppTypes(f, document):
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

#------------------------------------------------------------------------------
##
#
def WriteEnumeratedCsTypes(f, document):
    if "enums" in document:
        for enumName, enum in document["enums"].items():
            # Declare Enums
            f.WriteLine("public enum {}".format(enumName))
            f.WriteLine("{")
            f.IncreaseIndent()
            numValues = 0
            for key, value in enum.items():
                f.WriteLine("{} = {},".format(key, value))
                numValues += 1
            f.WriteLine("Num{} = {}".format(Capitalize(enumName), numValues))
            f.DecreaseIndent()
            f.WriteLine("}")
            f.WriteLine("")


#------------------------------------------------------------------------------
##
#
def WriteComponentCsDeclarations(f, document):
    for c in components:
        f.WriteLine("[NativeCppClass]")
        f.WriteLine("[StructLayout(LayoutKind.Sequential)]")
        f.WriteLine(c.AsCsTypeDefString())