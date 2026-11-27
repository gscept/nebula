import IDLC.idltypes as IDLTypes
import genutil as util
import IDLC.idldocument as IDLDocument

# Global component list
components = list()
structs = list()

#------------------------------------------------------------------------------
##
#
class VariableDefinition:
    def __init__(self, T, name: str, defVal, hideInInspector: bool, description: str, resourceType: str = ""):
        if not isinstance(T, str) and T is not None:
            util.fmtError('"type" value is not a string value!')
        self.type = T
        if not isinstance(name, str):
            util.fmtError('"name" value is not a string value!')
        if not isinstance(hideInInspector, bool):
            util.fmtError('"hideInInspector" value is not a bool value!')
        if not isinstance(description, str):
            util.fmtError('"description" value is not a string value!')
        if not isinstance(resourceType, str):
            util.fmtError('"resourceType" value is not a string value!')
        # The type of URN a field holds, folder:file:type, like "mdl". Only a field that holds a
        # string can name a resource
        if resourceType and IDLTypes.GetCppTypeString(T) not in ("Resources::ResourceName", "Util::StringAtom"):
            util.fmtError('"resourceType" is set on {}, which is a {} and not a resource or a string'.format(name, T))

        self.description = description
        self.hideInInspector = hideInInspector
        self.resourceType = resourceType
        self.name = name
        self.defaultValue = defVal

    def AsString(self):
        typeName = IDLTypes.GetCppTypeString(self.type)
        if self.defaultValue is None:
            return '{} {};'.format(typeName, self.name)
        else:
            return '{} {} = {};'.format(typeName, self.name, self.defaultValue)
    pass

    def CsType(self):
        return IDLTypes.GetCsTypeString(self.type).replace("::", ".")

    def AsCsString(self):
        return '{} {};'.format(self.CsType(), self.name)
    pass

#------------------------------------------------------------------------------
##
#
class ComponentDefinition:
    def __init__(self, componentName, comp):
        self.componentName = componentName
        self.variables = list()
        self.hasResource = False
        self.allowArray = comp["_allowArray_"] if "_allowArray_" in comp else False
        self.category = comp["category"] if "category" in comp else None
        
        if isinstance(comp, dict):
            for varName, var in comp.items():
                if varName != "_managed_" and varName != "_allowArray_":
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

#------------------------------------------------------------------------------
##
#
def AlignUp(value, align):
    return ((value + align - 1) // align) * align

#------------------------------------------------------------------------------
##
#   The largest power of two that fits in n. Marshal.SizeOf is the highest field
#   offset plus that field's size, and a field cannot be split across bytes, so the
#   trailing padding that brings a managed struct up to the native size has to be a
#   single field of that size.
#
def LargestPowerOfTwo(n):
    p = 0
    while (1 << p) <= n:
        p += 1
    return 1 << (p - 1) if p > 0 else 0

#------------------------------------------------------------------------------
##
#
def PadCsType(size):
    if size == 8:
        return "ulong"
    elif size == 4:
        return "uint"
    elif size == 2:
        return "ushort"
    return "byte"

#------------------------------------------------------------------------------
##
#   The native field offsets for a component, using the C struct layout rules.
#   Returns (entries, nativeSize) where entries is (variable, offset, size), or None
#   when a field has no blittable managed mirror and the layout cannot be computed.
#
def ComputeCsLayout(comp, document):
    entries = list()
    cursor = 0
    maxAlign = 1
    for v in comp.variables:
        nat = IDLTypes.GetNativeLayout(v.type, document)
        if nat is None:
            return None
        size, align = nat
        cursor = AlignUp(cursor, align)
        entries.append((v, cursor, size))
        cursor += size
        if align > maxAlign:
            maxAlign = align

    # An empty component is still one byte in C++, and Marshal.SizeOf of an empty
    # struct is 0, so the padding below covers it the same way it covers the rest
    return entries, max(1, AlignUp(cursor, maxAlign))

#------------------------------------------------------------------------------
##
#   The field description as an XML doc comment, which is what IntelliSense shows.
#   A doc comment is XML, so the markup characters have to come through as text,
#   and a description that spans lines becomes one /// line per line.
#
def EscapeCsDocText(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

#------------------------------------------------------------------------------
##
#
def WriteCsFieldComment(f, v):
    if not v.description:
        return

    lines = [EscapeCsDocText(line.strip()) for line in v.description.splitlines()]
    lines = [line for line in lines if line]
    if not lines:
        return

    if len(lines) == 1:
        f.WriteLine("/// <summary>{}</summary>".format(lines[0]))
    else:
        f.WriteLine("/// <summary>")
        for line in lines:
            f.WriteLine("/// {}".format(line))
        f.WriteLine("/// </summary>")

#------------------------------------------------------------------------------
##
#   Emit a struct with the native layout. C# cannot align a field to 16 the way
#   Math::mat4 and the SIMD vector types do, and Marshal treats a bool as 4 bytes
#   where native bool is 1, so the offsets have to be stated rather than derived.
#
def WriteCsStructLayout(f, comp, document, declaration):
    layout = ComputeCsLayout(comp, document)
    if layout is None:
        # A field the generator cannot size is not blittable, so there is no layout
        # to reproduce and the sequential form is left as it was
        f.WriteLine("[StructLayout(LayoutKind.Sequential)]")
        f.WriteLine(declaration)
        f.WriteLine("{")
        f.IncreaseIndent()
        for v in comp.variables:
            WriteCsFieldComment(f, v)
            f.WriteLine("public {}".format(v.AsCsString()))
        f.DecreaseIndent()
        f.WriteLine("}")
        return

    entries, nativeSize = layout
    managedMax = max([off + size for (_, off, size) in entries], default=0)

    f.WriteLine("[StructLayout(LayoutKind.Explicit)]")
    f.WriteLine(declaration)
    f.WriteLine("{")
    f.IncreaseIndent()
    for v, off, size in entries:
        WriteCsFieldComment(f, v)
        marshal = "[MarshalAs(UnmanagedType.U1)] " if v.CsType() == "bool" else ""
        f.WriteLine("[FieldOffset({})] {}public {} {};".format(off, marshal, v.CsType(), v.name))
    if managedMax < nativeSize:
        pad = LargestPowerOfTwo(nativeSize - managedMax)
        f.WriteLine("[FieldOffset({})] public {} __pad;".format(nativeSize - pad, PadCsType(pad)))
    f.DecreaseIndent()
    f.WriteLine("}")

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
        hideInInspector = False
        description = ""
        
        if "default" in var:
            default = IDLTypes.GetCppTypeString(var["type"]) + "(" + IDLTypes.DefaultToString(var["default"]) + ")"
        else:
            default = IDLTypes.DefaultValue(var["type"])

        if "hideInInspector" in var:
            hideInInspector = var["hideInInspector"]

        if "description" in var:
            description = var["description"]

        resourceType = ""
        if "resourceType" in var:
            resourceType = var["resourceType"]
        
        return VariableDefinition(var["type"], name, default, hideInInspector, description, resourceType)
    else:
        if hasattr(var, "__len__"):
            return VariableDefinition(var, name, None, False, "")
        else:
            return VariableDefinition(var, name, IDLTypes.DefaultValue(var), False, "")

#------------------------------------------------------------------------------
##
#
def ParseComponents(document):
    if "components" in document:
        for componentName, comp in document["components"].items():
            components.append(ComponentDefinition(componentName, comp))

    if "structs" in document:
        for structName, struct in document["structs"].items():
            structs.append(ComponentDefinition(structName, struct))

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
def ContainsColor():
    for comp in components:
        for var in comp.variables:
            if var.type == "color":
                return True
    return False


#------------------------------------------------------------------------------
##
#
def ContainsBitfieldTypes():
    for comp in components:
        for var in comp.variables:
            if var.type == "bitfield16" or var.type == "bitfield32" or var.type == "bitfield64":
                return True
    return False

#------------------------------------------------------------------------------
##
#   The layout the generator assumed, checked against the compiler. The size table in
#   idltypes.py is a hand-maintained guess about the C++ type, and the generated C#
#   struct is built from the same numbers, so these asserts are what says the two
#   sides still agree. The message is what the build prints when they stop agreeing.
#
def WriteNativeLayoutAsserts(f, comp, document, namespace):
    layout = ComputeCsLayout(comp, document)
    if layout is None:
        # A field the generator cannot size has no layout to check
        return

    entries, nativeSize = layout
    sizeMessage = "NIDL expected {} for {}.{}. If this assert fails, the size and alignment table GetNativeLayout in fips-files/generators/IDLC/idltypes.py does not match the C++ type: a field changed size or alignment, or a type used by this component is missing from the table. The generated C# struct is built from the same numbers, so the managed mirror is not byte-identical to this component.".format(nativeSize, namespace, comp.componentName)
    f.WriteLine('static_assert(sizeof({}) == {}, "{}");'.format(comp.componentName, nativeSize, sizeMessage))

    for v, off, size in entries:
        fieldMessage = "NIDL expected {} for {}.{}.{}. If this assert fails, the size and alignment table GetNativeLayout in fips-files/generators/IDLC/idltypes.py does not match the C++ type for this field or for a field before it. The generated C# struct is built from the same numbers, so the managed mirror reads this field at the wrong offset.".format(off, namespace, comp.componentName, v.name)
        f.WriteLine('static_assert(offsetof({}, {}) == {}, "{}");'.format(comp.componentName, v.name, off, fieldMessage))

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
        WriteNativeLayoutAsserts(f, c, document, namespace)
        f.WriteLine("Traits() = delete;")
        f.WriteLine('using type = {};'.format(c.componentName))
        f.WriteLine('static constexpr auto name = "{}";'.format(c.componentName))
        f.WriteLine('static constexpr auto namespace_name = "{}";'.format(namespace))
        f.WriteLine('static constexpr auto fully_qualified_name = "{}.{}";'.format(namespace, c.componentName))
        f.WriteLine('static constexpr size_t num_fields = {};'.format(len(c.variables)))
        if (len(c.variables) > 0):
            f.WriteLine('static constexpr const char* field_names[num_fields] = {')
            for v in c.variables:
                f.WriteLine('    "{}",'.format(v.name))
            f.WriteLine('};')
            f.WriteLine('static constexpr const char* field_typenames[num_fields] = {')
            for i, v in enumerate(c.variables):
                f.Write('    "{}"'.format(IDLTypes.GetCppTypeString(v.type)))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
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
            f.WriteLine('static constexpr const char* field_descriptions[num_fields] = {')
            for i, v in enumerate(c.variables):
                if not v.description:
                    f.Write('    nullptr')
                else:
                    f.Write('    "{}"'.format(v.description))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('};')
            f.WriteLine('static constexpr const char* const* field_enum_names[num_fields] = {')
            for i, v in enumerate(c.variables):
                enumName = GetEnumNameForType(document, v.type)
                f.Write('    {}'.format(enumName + "Names" if enumName else "nullptr"))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('};')
            f.WriteLine('static constexpr size_t field_enum_counts[num_fields] = {')
            for i, v in enumerate(c.variables):
                enumName = GetEnumNameForType(document, v.type)
                f.Write('    {}'.format(len(document["enums"][enumName]) if enumName else 0))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('};')
            f.WriteLine('static constexpr const int* field_enum_values[num_fields] = {')
            for i, v in enumerate(c.variables):
                enumName = GetEnumNameForType(document, v.type)
                f.Write('    {}'.format(enumName + "Values" if enumName else "nullptr"))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('};')
            f.WriteLine('static constexpr bool field_hide_in_inspector[num_fields] = {')
            for i, v in enumerate(c.variables):
                f.Write('    {}'.format("true" if v.hideInInspector else "false"))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('};')
            # the URN type a resource field holds, like "mdl", nullptr for a field that is not a
            # resource or takes any
            f.WriteLine('static constexpr const char* field_resource_types[num_fields] = {')
            for i, v in enumerate(c.variables):
                f.Write('    {}'.format('"{}"'.format(v.resourceType) if v.resourceType else "nullptr"))
                if i < (len(c.variables) - 1):
                    f.WriteLine(",")
                else:
                    f.WriteLine("")
            f.WriteLine('};')
        else:
            f.WriteLine('static constexpr const char** field_names = nullptr;')
            f.WriteLine('static constexpr const char** field_typenames = nullptr;')
            f.WriteLine('static constexpr size_t* field_byte_offsets = nullptr;')
            f.WriteLine('static constexpr const char** field_descriptions = nullptr;')
            f.WriteLine('static constexpr const char* const* field_enum_names = nullptr;')
            f.WriteLine('static constexpr size_t* field_enum_counts = nullptr;')
            f.WriteLine('static constexpr const int* field_enum_values = nullptr;')
            f.WriteLine('static constexpr bool* field_hide_in_inspector = nullptr;')
            f.WriteLine('static constexpr const char** field_resource_types = nullptr;')

        if c.category:  
            f.WriteLine(f'static constexpr const char* category = "{c.category}";')
        else:
            f.WriteLine('static constexpr const char* category = nullptr;')


        
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
        f.WriteLine('this->Add<int>((int)value, attr);')
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

#------------------------------------------------------------------------------
##
#
def WriteStructJsonSerializers(f, document):
    namespace = IDLDocument.GetNamespace(document)

    for struct in structs:
        f.WriteLine('template<> void JsonReader::Get<{namespace}::{name}>({namespace}::{name}& ret, const char* attr)'.format(namespace=namespace, name=struct.componentName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine('ret = {namespace}::{name}();'.format(namespace=namespace, name=struct.componentName))
        f.WriteLine("const pjson::value_variant* node = this->GetChild(attr);")
        f.WriteLine("if (node->is_object())")
        f.WriteLine("{")
        f.IncreaseIndent()
        f.WriteLine("this->SetToNode(attr);")
        for var in struct.variables:
            if IDLTypes.IsStdArrayType(var.type):
                f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name))
            else:
                f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get<{type}>(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetJsonParserTypeString(var.type)))
        f.WriteLine("this->SetToParent();")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

        f.WriteLine('template<> void JsonWriter::Add<{namespace}::{name}>({namespace}::{name} const& value, Util::String const& attr)'.format(namespace=namespace, name=struct.componentName))
        f.WriteLine('{')
        f.IncreaseIndent()
        f.WriteLine("this->BeginObject(attr.AsCharPtr());")
        for var in struct.variables:
            if IDLTypes.IsStdArrayType(var.type):
                f.WriteLine('this->Add(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name))
            else:
                f.WriteLine('this->Add<{type}>(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetJsonParserTypeString(var.type)))
        f.WriteLine("this->End();")
        f.DecreaseIndent()
        f.WriteLine("}")
        f.WriteLine("")

        if struct.allowArray:
            f.WriteLine('template<> void JsonReader::Get<Util::Array<{namespace}::{name}>>(Util::Array<{namespace}::{name}>& ret, const char* attr)'.format(namespace=namespace, name=struct.componentName))
            f.WriteLine('{')
            f.IncreaseIndent()
            f.WriteLine('ret = Util::Array<{namespace}::{name}>();'.format(namespace=namespace, name=struct.componentName))
            f.WriteLine("const pjson::value_variant* node = this->GetChild(attr);")
            f.WriteLine("n_assert(node->is_array());")
            f.WriteLine("unsigned int count = node->size();")
            f.WriteLine("ret.Reserve(count);")
            f.WriteLine("for (unsigned int i = 0; i < count; i++)")
            f.WriteLine("{")
            f.IncreaseIndent()
            f.WriteLine('{namespace}::{name} element;'.format(namespace=namespace, name=struct.componentName))
            f.WriteLine('this->Get<{namespace}::{name}>(element, node->get_key_name_at_index(i));'.format(namespace=namespace, name=struct.componentName))
            f.WriteLine("ret.Append(element);")
            f.DecreaseIndent()
            f.WriteLine("}")
            f.DecreaseIndent()
            f.WriteLine("}")
            f.WriteLine("")

            f.WriteLine('template<> void JsonWriter::Add<Util::Array<{namespace}::{name}>>(Util::Array<{namespace}::{name}> const& values, Util::String const& attr)'.format(namespace=namespace, name=struct.componentName))
            f.WriteLine('{')
            f.IncreaseIndent()
            f.WriteLine("this->BeginArray(attr.AsCharPtr());")
            f.WriteLine('for (const auto& value : values)')
            f.WriteLine("{")
            f.IncreaseIndent()
            for var in struct.variables:
                f.WriteLine('this->Add<{type}>(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetCppTypeString(var.type)))
            f.DecreaseIndent()
            f.WriteLine("}")
            f.WriteLine("this->End();")
            f.DecreaseIndent()
            f.WriteLine("}")
        f.WriteLine("")

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
            if IDLTypes.IsStdArrayType(var.type):
                f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name))
            else:
                f.WriteLine('if (this->HasAttr("{fieldName}")) this->Get<{type}>(ret.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetJsonParserTypeString(var.type)))
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
            if IDLTypes.IsStdArrayType(var.type):
                f.WriteLine('this->Add(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name))
            else:
                f.WriteLine('this->Add<{type}>(value.{fieldName}, "{fieldName}");'.format(fieldName=var.name, type=IDLTypes.GetJsonParserTypeString(var.type)))
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
            f.WriteLine("enum class {} : int32_t".format(enumName))
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

            # Value names ordered by value. The component inspector uses these to draw enum fields as combos.
            ordered = sorted(enum.items(), key=lambda item: item[1])
            f.WriteLine('static constexpr const char* const {}Names[] = {{'.format(enumName))
            for key, value in ordered:
                f.WriteLine('    "{}",'.format(key))
            f.WriteLine('};')
            f.WriteLine('static constexpr size_t {}NameCount = {};'.format(enumName, len(ordered)))
            # The values at the same indices as the names, so combo indices and enum values can
            # differ for enums that are not contiguous from zero
            f.WriteLine('static constexpr int {}Values[] = {{'.format(enumName))
            for key, value in ordered:
                f.WriteLine('    {},'.format(value))
            f.WriteLine('};')
            f.WriteLine("")

#------------------------------------------------------------------------------
##
#
def GetEnumNameForType(document, attrType):
    namespace = IDLDocument.GetNamespace(document)
    if "enums" in document:
        for enumName in document["enums"]:
            if attrType == "{}::{}".format(namespace, enumName):
                return enumName
    return None

#------------------------------------------------------------------------------
##
#
def WriteStructCppTypes(f, document):
	if 'structs' in document:
		for structName, struct in document['structs'].items():
			f.WriteLine("struct {} \n{{".format(structName))
			f.IncreaseIndent()
			for varName, var in struct.items():
				if varName != '_managed_' and varName != '_allowArray_':
					varDef = GetVariableFromEntry(varName, var)
					f.WriteLine(varDef.AsString())
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
def WriteStructCsTypes(f, document):
	if 'structs' in document:
		for structName, struct in document['structs'].items():
			f.WriteLine("[NativeCppClass]")
			WriteCsStructLayout(f, ComponentDefinition(structName, struct), document, "public struct {}".format(structName))
			f.WriteLine("")

#------------------------------------------------------------------------------
##
#
def WriteComponentCsDeclarations(f, document):
    for c in components:
        f.WriteLine("[NativeCppClass]")
        WriteCsStructLayout(f, c, document, "public struct {} : NativeComponent".format(c.componentName))
