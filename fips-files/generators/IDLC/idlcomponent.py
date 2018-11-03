import genutil as util
import idltypes as IDLTypes

AttributeNotFoundError = 'No attribute named {} could be found by component compiler! Please make sure it\'s defined or imported as a dependency in the current .nidl file.'

def Capitalize(s):
    return s[:1].upper() + s[1:]

#------------------------------------------------------------------------------
##
#
def WriteIncludes(f, attributeLibraries):
    f.WriteLine('#include "game/component/component.h"')

    for lib in attributeLibraries:
        f.WriteLine('#include "{}"'.format(lib))


class ComponentClassWriter:
    def __init__(self, fileWriter, document, component, componentName, namespace):
        self.f = fileWriter
        self.document = document
        self.component = component
        self.componentName = componentName
        self.className = '{}Base'.format(self.componentName)
        self.namespace = namespace

        if (not "fourcc" in component):
            util.fmtError('Component does contain a FourCC')

        self.fourcc = component["fourcc"]
        self.useDelayedRemoval = "useDelayedRemoval" in component and component["useDelayedRemoval"] == True

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
                self.componentTemplateArguments += "decltype(Attr::{})".format(Capitalize(attributeName))
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
    __DeclareClass({className})

public:
    /// Default constructor
    {className}();
    /// Default destructor
    ~{className}();

    enum AttributeIndex
    {{
        OWNER,
{enumAttributeList}
        NumAttributes
    }};

    /// Registers an entity to this component.
    uint32_t RegisterEntity(const Game::Entity& entity);

    /// Deregister Entity.
    void DeregisterEntity(const Game::Entity& entity);

    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();

    /// Optimize data array and pack data
    SizeT Optimize();

    /// Called from entitymanager if this component is registered with a deletion callback.
    /// Removes entity immediately from component instances.
    void OnEntityDeleted(Game::Entity entity);
}};
        """.format(
            className=self.className,
            enumAttributeList=self.GetEnumAttributeList(),
            componentTemplateArguments=self.componentTemplateArguments
        )

        self.f.WriteLine(headerTemplate)

    #------------------------------------------------------------------------------
    ##
    #
    def GetEnumAttributeList(self):
        retval = ""
        if self.hasAttributes:
            for attributeName in self.component["attributes"]:
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                retval += "        {},\n".format(attributeName.upper())
        return retval

    #------------------------------------------------------------------------------
    ##
    #
    def WriteConstructorImplementation(self):
        numAttributes = 0
        if self.hasAttributes:
            numAttributes += len(self.component["attributes"])

        self.f.WriteLine("__ImplementClass({}::{}, '{}', Core::RefCounted)".format(self.namespace, self.className, self.fourcc))
        self.f.WriteLine("")
        self.f.InsertNebulaDivider()
        self.f.WriteLine("{className}::{className}() :".format(className=self.className))
        self.f.IncreaseIndent()
        self.f.WriteLine("component_templated_t({")
        self.f.IncreaseIndent()
        if self.hasAttributes:
            for i, attributeName in enumerate(self.component["attributes"]):
                self.f.Write("Attr::{}".format(Capitalize(attributeName)))
                if i != numAttributes:
                    self.f.WriteLine(",")
                else:
                    self.f.WriteLine("")
        self.f.DecreaseIndent()
        self.f.WriteLine("})")
        self.f.DecreaseIndent()
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        if self.hasEvents:
            for event in self.component["events"]:
                self.f.WriteLine("this->events.SetBit({});".format(IDLTypes.GetEventEnum(event)))
            self.f.WriteLine("")
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
    def WriteRegisterEntityImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("uint32_t")
        self.f.WriteLine("{}::RegisterEntity(const Game::Entity& entity)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("auto instance = component_templated_t::RegisterEntity(entity);")

        if not self.useDelayedRemoval:
            self.f.WriteLine("Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);")

        if self.hasEvents and "onactivate" in self.events:
            self.f.WriteLine("")
            self.f.WriteLine("this->OnActivate(instance);")

        self.f.WriteLine("return instance;")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteDeregisterEntityImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::DeregisterEntity(const Game::Entity& entity)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("uint32_t index = this->GetInstance(entity);")
        self.f.WriteLine("if (index != InvalidIndex)")
        self.f.WriteLine("{")
        self.f.IncreaseIndent()

        if self.hasEvents and "ondeactivate" in self.events:
            self.f.WriteLine("this->OnDeactivate(index);")
            self.f.WriteLine("")

        if self.useDelayedRemoval:
            self.f.WriteLine("component_templated_t::DeregisterEntity(entity);")
        else:
            self.f.WriteLine("this->DeregisterEntityImmediate(entity);")
            self.f.WriteLine("Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);")

        self.f.WriteLine("return;")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteOnEntityDeletedImplementation(self):
            self.f.InsertNebulaDivider()
            self.f.WriteLine("void")
            self.f.WriteLine("{}::OnEntityDeleted(Game::Entity entity)".format(self.className))
            self.f.WriteLine("{")
            self.f.IncreaseIndent()
            if not self.useDelayedRemoval:
                self.f.WriteLine("uint32_t index = this->GetInstance(entity);")
                self.f.WriteLine("if (index != InvalidIndex)")
                self.f.WriteLine("{")
                self.f.IncreaseIndent()
                self.f.WriteLine("this->DeregisterEntityImmediate(entity);")
                self.f.WriteLine("return;")
                self.f.DecreaseIndent()
                self.f.WriteLine("}")
            else:
                self.f.WriteLine("return;")
            self.f.DecreaseIndent()
            self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteOptimizeImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("SizeT")
        self.f.WriteLine("{}::Optimize()".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()

        if self.useDelayedRemoval:
            self.f.WriteLine("return component_templated_t::Optimize();")
        else:
            self.f.WriteLine("return 0;")

        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteDestroyAllImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::DestroyAll()".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        if not self.useDelayedRemoval:
            self.f.WriteLine("SizeT length = this->data.Size();")
            self.f.WriteLine("for (SizeT i = 0; i < length; i++)")
            self.f.WriteLine("{")
            self.f.WriteLine("    Game::EntityManager::Instance()->DeregisterDeletionCallback(this->GetOwner(i), this);")
            self.f.WriteLine("}")
        
        self.f.WriteLine("component_templated_t::DestroyAll();")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        



    #------------------------------------------------------------------------------
    ##
    #
    def WriteClassImplementation(self):
        self.WriteConstructorImplementation()
        self.WriteDestructorImplementation()
        self.WriteRegisterEntityImplementation()
        self.WriteDeregisterEntityImplementation()
        self.WriteDestroyAllImplementation()
        self.WriteOptimizeImplementation()
        self.WriteOnEntityDeletedImplementation()
