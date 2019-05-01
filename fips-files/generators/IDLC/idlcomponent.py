import genutil as util
import IDLC.idltypes as IDLTypes

AttributeNotFoundError = 'No attribute named {} could be found by component compiler! Please make sure it\'s defined or imported as a dependency in the current .nidl file.'

def Capitalize(s):
    return s[:1].upper() + s[1:]

#------------------------------------------------------------------------------
##
#
def WriteIncludes(f, attributeLibraries):
    for lib in attributeLibraries:
        f.WriteLine('#include "{}"'.format(lib))


class ComponentClassWriter:
    def __init__(self, fileWriter, document, component, componentName, namespace):
        self.f = fileWriter
        self.document = document
        self.component = component
        self.componentName = componentName
        self.className = '{}Allocator'.format(self.componentName)
        self.namespace = namespace

        if (not "fourcc" in component):
            util.fmtError('Component does contain a FourCC')

        self.fourcc = component["fourcc"]
        self.incrementalDeletion = "incrementalDeletion" in component and component["incrementalDeletion"] == True

        self.hasEvents = "events" in component
        if self.hasEvents:
            self.events = self.component["events"]
            for i, event in enumerate(self.events):
                self.events[i] = event.lower()

        self.hasAttributes = "attributes" in self.component

        if self.hasAttributes and not "attributes" in self.document:
            util.fmtError('Component has attributes attached but none could be found by the component compiler! Please make sure they\'re defined or imported as a dependency in the current .nidl file.')

        self.componentTemplateArguments = ""
        if (self.hasAttributes):
            numAttributes = len(self.component["attributes"])
            for i, attributeName in enumerate(self.component["attributes"]):
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                self.componentTemplateArguments += "Attr::{}".format(Capitalize(attributeName))
                if i != (numAttributes - 1):
                    self.componentTemplateArguments += ",\n    "

    #------------------------------------------------------------------------------
    ##
    #
    def WriteClassDeclaration(self):
        headerTemplate = """
class {className} : public Game::Component<
    {componentTemplateArguments}
>
{{
public:
    /// Default constructor
    {className}();
    /// Default destructor
    ~{className}();

    /// Returns the components fourcc
    static Util::FourCC GetIdentifier()
    {{
        return '{fourcc}';
    }}

        """.format(
            className=self.className,
            fourcc=self.fourcc,
            componentTemplateArguments=self.componentTemplateArguments
        )
        self.f.WriteLine(headerTemplate)
        self.f.IncreaseIndent()
        self.WriteAttributeAccessDeclarations()
        self.f.DecreaseIndent()
        self.f.WriteLine("};")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteAttributeAccessDeclarations(self):
        if self.hasAttributes:
            self.f.WriteLine("/// Attribute access methods")
            for attributeName in self.component["attributes"]:
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                
                returnType = IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"])
                attributeName = Capitalize(attributeName)

                self.f.WriteLine("{retval}& {attributeName}(Game::InstanceId instance);".format(retval=returnType, attributeName=attributeName))

    #------------------------------------------------------------------------------
    ##
    #
    def WriteAttributeAccessImplementation(self):
        if self.hasAttributes:
            for i, attributeName in enumerate(self.component["attributes"]):
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                
                returnType = IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"])
                attributeName = Capitalize(attributeName)

                self.f.InsertNebulaDivider()
                self.f.WriteLine("{retval}& {className}::{attributeName}(Game::InstanceId instance)".format(retval=returnType, className=self.className, attributeName=attributeName))
                self.f.WriteLine("{")
                self.f.IncreaseIndent()
                self.f.WriteLine("return this->data.Get<{}>(instance);".format(i + 1))
                self.f.DecreaseIndent()
                self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteConstructorImplementation(self):
        numAttributes = 0
        if self.hasAttributes:
            numAttributes += len(self.component["attributes"])

        # self.f.WriteLine("__ImplementWeakClass({}::{}, '{}', Game::ComponentInterface);".format(self.namespace, self.className, self.fourcc))
        # self.f.WriteLine("__RegisterClass({})".format(self.className))

        self.f.InsertNebulaDivider()
        self.f.WriteLine("{className}::{className}()".format(className=self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        if self.hasEvents:
            for event in self.component["events"]:
                self.f.WriteLine("this->events.SetBit({});".format(IDLTypes.GetEventEnum(event)))
            self.f.WriteLine("")

        if self.incrementalDeletion:
            self.f.WriteLine("this->settings.incrementalDeletion = true;")
        
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteDestructorImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("{}::~{}()".format(self.className, self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("// empty")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteClassImplementation(self):
        self.WriteConstructorImplementation()
        self.WriteDestructorImplementation()
        self.WriteAttributeAccessImplementation()
