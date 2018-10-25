import genutil as util
import idltypes as IDLTypes

AttributeNotFoundError = 'No attribute named {} could be found by component compiler! Please make sure it\'s defined or imported as a dependency in the current .nidl file.'

def Capitalize(s):
    return s[:1].upper() + s[1:]

#------------------------------------------------------------------------------
##
#
def WriteIncludes(f, attributeLibraries):
    f.WriteLine('#include "game/component/componentdata.h"')
    f.WriteLine('#include "game/component/basecomponent.h"')
    
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
        
        self.componentDataArguments = ""
        if (self.hasAttributes):
            numAttributes = len(self.component["attributes"])
            for i, attributeName in enumerate(self.component["attributes"]):
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                self.componentDataArguments += IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"])
                if i != (numAttributes - 1):
                    self.componentDataArguments += ", "

    #------------------------------------------------------------------------------
    ##
    #
    def WriteClassDeclaration(self):
        headerTemplate = """
class {className} : public Game::BaseComponent
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
    void RegisterEntity(const Game::Entity& entity);
    
    /// Deregister Entity.
    void DeregisterEntity(const Game::Entity& entity);
    
    /// Cleans up right away and frees any memory that does not belong to an entity. (slow!)
    void CleanData();
    
    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();
    
    /// Checks whether the entity is registered.
    bool IsRegistered(const Game::Entity& entity) const;
    
    /// Returns the index of the data array to the component instance
    uint32_t GetInstance(const Game::Entity& entity) const;
    
    /// Returns the owner entity id of provided instance id
    Game::Entity GetOwner(const uint32_t& instance) const;
    
    /// Set the owner of a given instance. This does not care if the entity is registered or not!
    void SetOwner(const uint32_t& i, const Game::Entity& entity);
    
    /// Optimize data array and pack data
    SizeT Optimize();
    
    /// Returns an attribute value as a variant from index.
    Util::Variant GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const;
    
    /// Returns an attribute value as a variant from attribute id.
    Util::Variant GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const;
    
    /// Set an attribute value from index
    void SetAttributeValue(uint32_t instance, AttributeIndex attributeIndex, Util::Variant value);
    
    /// Set an attribute value from attribute id
    void SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value);
    
    /// Serialize component into binary stream
    void Serialize(const Ptr<IO::BinaryWriter>& writer) const;

    /// Deserialize from binary stream and set data.
    void Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances);

    /// Get the total number of instances of this component
    uint32_t NumRegistered() const;

    /// Allocate multiple instances
    void Allocate(uint num);
    {onEntityDeletedDeclaration}

    {attributeSetGetDeclarations}

protected:
    {attributeOnUpdatedCallbacks}

private:
    /// Holds all entity instances data")
    Game::ComponentData<{componentDataArguments}> data;
}};
        """.format(
            className=self.className,
            enumAttributeList=self.GetEnumAttributeList(),
            onEntityDeletedDeclaration=self.GetEntityDeletedDeclaration(),
            attributeSetGetDeclarations=self.GetAttributeAccessDeclarations(),
            attributeOnUpdatedCallbacks=self.GetAttributeOnUpdatedDeclarations(),
            componentDataArguments=self.componentDataArguments
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
    def GetEntityDeletedDeclaration(self):
        if not self.useDelayedRemoval:
            return """
            /// Called from entitymanager if this component is registered with a deletion callback.
            /// Removes entity immediately from component instances.
            void OnEntityDeleted(Game::Entity entity);
            
            """
        else:
            return ""
    
    #------------------------------------------------------------------------------
    ##
    #
    def GetAttributeAccessDeclarations(self):
        retval = ""
        if self.hasAttributes:
            retval += "/// Read/write access to attributes.\n"
            for attributeName in self.component["attributes"]:
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                retval += '    const {}& Get{}(const uint32_t& instance);\n'.format(IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"]), Capitalize(attributeName))
                retval += '    void Set{}(const uint32_t& instance, const {}& value);\n'.format(Capitalize(attributeName), IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"]))
        return retval

    #------------------------------------------------------------------------------
    ##
    #
    def GetAttributeOnUpdatedDeclarations(self):
        retval = ""
        if self.hasAttributes:
            retval += "/// Callbacks for reacting to updated attributes.\n"
            for attributeName in self.component["attributes"]:
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                retval += '    virtual void On{}Updated(const uint32_t& instance, const {}& value);\n'.format(Capitalize(attributeName), IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"]))
        return retval

    #------------------------------------------------------------------------------
    ##
    #
    def WriteConstructorImplementation(self):
        self.f.WriteLine("__ImplementClass({}::{}, '{}', Core::RefCounted)".format(self.namespace, self.className, self.fourcc))
        self.f.WriteLine("")
        self.f.InsertNebulaDivider()
        self.f.WriteLine("{}::{}()".format(self.className, self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        if self.hasEvents:
            for event in self.component["events"]:
                self.f.WriteLine("this->events.SetBit({});".format(IDLTypes.GetEventEnum(event)))
            self.f.WriteLine("")

        numAttributes = 1
        if self.hasAttributes:
            numAttributes += len(self.component["attributes"])

        self.f.WriteLine("this->attributeIds.SetSize({});".format(numAttributes))

        self.f.WriteLine("this->attributeIds[0] = Attr::Owner;")

        if self.hasAttributes:
            for i, attributeName in enumerate(self.component["attributes"]):
                self.f.WriteLine("this->attributeIds[{}] = Attr::{};".format(i + 1, Capitalize(attributeName)))

        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

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
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteRegisterEntityImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::RegisterEntity(const Game::Entity& entity)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("auto instance = this->data.RegisterEntity(entity);")
        
        if self.hasAttributes:
            for attributeName in self.component["attributes"]:
                default = "Attr::{}.GetDefaultValue().Get{}()".format(Capitalize(attributeName), IDLTypes.ConvertToCamelNotation(self.document["attributes"][attributeName]["type"]))
                self.f.WriteLine("this->data.data.Get<{}>(instance) = {};".format(attributeName.upper(), default))

        if not self.useDelayedRemoval:
            self.f.WriteLine("Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);")

        if self.hasEvents and "onactivate" in self.events:
            self.f.WriteLine("")
            self.f.WriteLine("this->OnActivate(instance);")

        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteDeregisterEntityImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::DeregisterEntity(const Game::Entity& entity)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("uint32_t index = this->data.GetInstance(entity);")
        self.f.WriteLine("if (index != InvalidIndex)")
        self.f.WriteLine("{")
        self.f.IncreaseIndent()

        if self.hasEvents and "ondeactivate" in self.events:
            self.f.WriteLine("this->OnDeactivate(index);")
            self.f.WriteLine("")

        if self.useDelayedRemoval:
            self.f.WriteLine("this->data.DeregisterEntity(entity);")
        else:
            self.f.WriteLine("this->data.DeregisterEntityImmediate(entity);")
            self.f.WriteLine("Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);")

        self.f.WriteLine("return;")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteOnEntityDeletedImplementation(self):
        if not self.useDelayedRemoval:
            self.f.InsertNebulaDivider()
            self.f.WriteLine("void")
            self.f.WriteLine("{}::OnEntityDeleted(Game::Entity entity)".format(self.className))
            self.f.WriteLine("{")
            self.f.IncreaseIndent()
            self.f.WriteLine("uint32_t index = this->data.GetInstance(entity);")
            self.f.WriteLine("if (index != InvalidIndex)")
            self.f.WriteLine("{")
            self.f.IncreaseIndent()
            self.f.WriteLine("this->data.DeregisterEntityImmediate(entity);")
            self.f.WriteLine("return;")
            self.f.DecreaseIndent()
            self.f.WriteLine("}")
            self.f.DecreaseIndent()
            self.f.WriteLine("}")
            self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteCleanupMethods(self):
        self.f.InsertNebulaComment("@todo	if needed: deregister deletion callbacks")
        self.f.WriteLine("void")
        self.f.WriteLine("{}::CleanData()".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("this->data.Clean();")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")
        self.f.InsertNebulaComment("@todo	if needed: deregister deletion callbacks")
        self.f.WriteLine("void")
        self.f.WriteLine("{}::DestroyAll()".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("this->data.DestroyAll();")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteIsRegisteredImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("bool")
        self.f.WriteLine("{}::IsRegistered(const Game::Entity& entity) const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("return this->data.GetInstance(entity) != InvalidIndex;")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteGetInstanceImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("uint32_t")
        self.f.WriteLine("{}::GetInstance(const Game::Entity& entity) const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("return this->data.GetInstance(entity);")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteOwnerImplementation(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("Game::Entity")
        self.f.WriteLine("{}::GetOwner(const uint32_t& instance) const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("return this->data.GetOwner(instance);")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")
        
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::SetOwner(const uint32_t & i, const Game::Entity & entity)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("this->data.SetOwner(i, entity);")
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
            self.f.WriteLine("return this->data.Optimize();;")
        else:
            self.f.WriteLine("return 0;")

        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteGetAttributeMethod(self):
        if not self.hasAttributes:
            return

        self.f.InsertNebulaDivider()
        self.f.WriteLine("Util::Variant")
        self.f.WriteLine("{}::GetAttributeValue(uint32_t instance, AttributeIndex attributeIndex) const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("switch (attributeIndex)")
        self.f.WriteLine("{")
        self.f.IncreaseIndent()

        self.f.WriteLine("case 0: return Util::Variant(this->data.data.Get<0>(instance).id);")
        for i, attributeName in enumerate(self.component["attributes"]):
            index = i + 1
            self.f.Write("case {}: ".format(index))
            self.f.WriteLine("return Util::Variant(this->data.data.Get<{}>(instance));".format(index))
        self.f.WriteLine("default:")
        self.f.WriteLine("    n_assert2(false, \"Component doesn't contain this attribute!\");")
        self.f.WriteLine("    return Util::Variant();")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

        self.f.InsertNebulaDivider()
        self.f.WriteLine("Util::Variant")
        self.f.WriteLine("{}::GetAttributeValue(uint32_t instance, Attr::AttrId attributeId) const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("if (attributeId == Attr::Owner) return Util::Variant(this->data.data.Get<0>(instance).id);")
            
        for i, attributeName in enumerate(self.component["attributes"]):
            index = i + 1
            self.f.Write("else if (attributeId == Attr::{}) ".format(Capitalize(attributeName)))
            self.f.WriteLine("return Util::Variant(this->data.data.Get<{}>(instance));".format(index))
            
        self.f.WriteLine('n_assert2(false, "Component does not contain this attribute!");')
        self.f.WriteLine("return Util::Variant();")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteSetAttributeMethod(self):
        if not self.hasAttributes:
            return

        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::SetAttributeValue(uint32_t instance, AttributeIndex index, Util::Variant value)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("switch (index)")
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        for i, attributeName in enumerate(self.component["attributes"]):
            index = i + 1
            attribute = self.document["attributes"][attributeName]
            if attribute["access"].lower() == "r":
                continue
            self.f.Write("case {}: ".format(index))
            self.f.WriteLine("this->Set{}(instance, value.Get{}());".format(Capitalize(attributeName), IDLTypes.ConvertToCamelNotation(attribute["type"])))
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")

        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::SetAttributeValue(uint32_t instance, Attr::AttrId attributeId, Util::Variant value)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        printElse = False
        for i, attributeName in enumerate(self.component["attributes"]):
            index = i + 1
            attribute = self.document["attributes"][attributeName]
            if attribute["access"].lower() == "r":
                continue
            
            # kinda dirty, but it works
            if printElse:
                self.f.Write("else ")
            else:
                printElse = True

            self.f.Write("if (attributeId == Attr::{}) ".format(Capitalize(attributeName)))
            self.f.WriteLine("this->Set{}(instance, value.Get{}());".format(Capitalize(attributeName), IDLTypes.ConvertToCamelNotation(attribute["type"])))
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("")


    #------------------------------------------------------------------------------
    ## note     If we're ever having autosynced attributes, this is where to put the sync code
    #
    def WriteAttrAccessImplementations(self):
        if self.hasAttributes:
            for i, attributeName in enumerate(self.component["attributes"]):
                T = IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"])
                if not attributeName in self.document["attributes"]:
                    util.fmtError(AttributeNotFoundError.format(attributeName))
                self.f.InsertNebulaDivider()
                self.f.WriteLine("const {}&".format(T))
                self.f.WriteLine("{}::Get{}(const uint32_t& instance)".format(self.className, Capitalize(attributeName)))
                self.f.WriteLine("{")
                self.f.IncreaseIndent()
                self.f.WriteLine("return this->data.data.Get<{}>(instance);".format(i + 1))
                self.f.DecreaseIndent()
                self.f.WriteLine("}")
                self.f.WriteLine("")

                self.f.InsertNebulaDivider()
                self.f.WriteLine("void")
                self.f.WriteLine("{}::Set{}(const uint32_t& instance, const {}& value)".format(self.className, Capitalize(attributeName), T))
                self.f.WriteLine("{")
                self.f.IncreaseIndent()
                self.f.WriteLine("this->data.data.Get<{}>(instance) = value;".format(i + 1))
                self.f.WriteLine("// callback that we can hook into to react to this change")
                self.f.WriteLine("this->On{}Updated(instance, value);".format(Capitalize(attributeName)));
                self.f.DecreaseIndent()
                self.f.WriteLine("}")
                self.f.WriteLine("")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteSerializationMethods(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::Serialize(const Ptr<IO::BinaryWriter>& writer) const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("this->data.Serialize(writer);")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")
        self.f.WriteLine("");
        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("this->data.Deserialize(reader, offset, numInstances);")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteAllocInstancesMethod(self):
        self.f.InsertNebulaDivider()
        self.f.WriteLine("uint32_t")
        self.f.WriteLine("{}::NumRegistered() const".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("return this->data.Size();")
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

        self.f.InsertNebulaDivider()
        self.f.WriteLine("void")
        self.f.WriteLine("{}::Allocate(uint num)".format(self.className))
        self.f.WriteLine("{")
        self.f.IncreaseIndent()
        self.f.WriteLine("SizeT first = this->data.data.Size();")
        self.f.WriteLine("this->data.data.Reserve(first + num);")
        self.f.WriteLine("this->data.data.GetArray<OWNER>().SetSize(first + num);")

        if self.hasAttributes:
            for attributeName in self.component["attributes"]:
                default = "Attr::{}.GetDefaultValue().Get{}()".format(Capitalize(attributeName), IDLTypes.ConvertToCamelNotation(self.document["attributes"][attributeName]["type"]))
                self.f.WriteLine("this->data.data.GetArray<{}>().Fill(first, num, {});".format(attributeName.upper(), default))

        self.f.WriteLine("this->data.data.UpdateSize();");
        self.f.DecreaseIndent()
        self.f.WriteLine("}")

    #------------------------------------------------------------------------------
    ##
    #
    def WriteCallbackMethods(self):
        if self.hasAttributes:
            for attributeName in self.component["attributes"]:
                self.f.WriteLine("");
                self.f.InsertNebulaDivider()
                self.f.WriteLine("void")
                self.f.WriteLine('{}::On{}Updated(const uint32_t& instance, const {}& value)'.format(self.className, Capitalize(attributeName), IDLTypes.GetTypeString(self.document["attributes"][attributeName]["type"])))
                self.f.WriteLine("{")
                self.f.IncreaseIndent()
                self.f.WriteLine("// Empty - override if necessary")
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
        self.WriteCleanupMethods()
        self.WriteIsRegisteredImplementation()
        self.WriteGetInstanceImplementation()
        self.WriteOwnerImplementation()
        self.WriteOptimizeImplementation()
        self.WriteGetAttributeMethod()
        self.WriteSetAttributeMethod()
        self.WriteSerializationMethods()
        self.WriteOnEntityDeletedImplementation()
        self.WriteAttrAccessImplementations()
        self.WriteAllocInstancesMethod()
        self.WriteCallbackMethods();
        


