import os, platform, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import idldocument as IDLDocument
import idlattribute as IDLAttribute
import idlcomponent as IDLComponent
import sjson
import filewriter
#import genutil as util
import ntpath

class IDLCodeGenerator:
    def __init__(self):
        self.document = None
        self.documentPath = ""
        self.version = 0;

    #------------------------------------------------------------------------------
    ##
    #
    def SetVersion(self, v):
        self.version = v

    #------------------------------------------------------------------------------
    ##
    #
    def SetDocument(self, input) :
        self.documentPath = input
        self.documentBaseName = os.path.splitext(input)[0]
        self.documentDirName = os.path.dirname(self.documentBaseName)

        head, tail = ntpath.split(self.documentBaseName)
        self.documentFileName = tail or ntpath.basename(head)

        fstream = open(self.documentPath, 'r')
        self.document = sjson.loads(fstream.read())
        fstream.close()


    #------------------------------------------------------------------------------
    ##
    #
    def GenerateHeader(self, hdrPath) :
        f = filewriter.FileWriter()
        f.Open(hdrPath)

        f.WriteLine("// NIDL #version:{}#".format(self.version))

        attributeLibraries = []

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fileName = '{}.h'.format(os.path.splitext(dependency)[0]).lower()
                attributeLibraries.append(fileName)

        IDLDocument.WriteIncludeHeader(f)
        IDLComponent.WriteIncludes(f, attributeLibraries)


        # Generate attributes include file
        if "attributes" in self.document:
            IDLDocument.WriteAttributeLibraryDeclaration(f)
            
            if "enums" in self.document:
                IDLDocument.BeginNamespace(f, self.document)
                IDLAttribute.WriteEnumeratedTypes(f, self.document)
                IDLDocument.EndNamespace(f, self.document)
                f.WriteLine("")

            IDLDocument.BeginNamespaceOverride(f, self.document, "Attr")
            IDLAttribute.WriteAttributeHeaderDeclarations(f, self.document)
            IDLDocument.EndNamespaceOverride(f, self.document, "Attr")
            f.WriteLine("")

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fstream = open(dependency, 'r')
                depDocument = sjson.loads(fstream.read())
                deps = depDocument["attributes"]
                # Add all attributes to this document
                self.document["attributes"].update(deps)
                fstream.close()

        # Generate components base classes headers
        if "components" in self.document:
            IDLDocument.BeginNamespace(f, self.document)
            namespace = IDLDocument.GetNamespace(self.document)
            for componentName, component in self.document["components"].iteritems():
                componentWriter = IDLComponent.ComponentClassWriter(f, self.document, component, componentName, namespace)
                componentWriter.WriteClassDeclaration()
            IDLDocument.EndNamespace(f, self.document)
        
        f.Close()


    #------------------------------------------------------------------------------
    ##
    #
    def GenerateSource(self, srcPath, hdrPath) :
        f = filewriter.FileWriter()
        f.Open(srcPath)

        f.WriteLine("// NIDL #version:{}#".format(self.version))

        head, tail = ntpath.split(hdrPath)
        hdrInclude = tail or ntpath.basename(head)

        head, tail = ntpath.split(srcPath)
        srcFileName = tail or ntpath.basename(head)

        IDLDocument.WriteSourceHeader(f, srcFileName)
        IDLDocument.AddInclude(f, hdrInclude)

        if "attributes" in self.document:
            IDLDocument.BeginNamespaceOverride(f, self.document, "Attr")
            IDLAttribute.WriteAttributeDefinitions(f, self.document)
            IDLDocument.EndNamespaceOverride(f, self.document, "Attr")

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fstream = open(dependency, 'r')
                depDocument = sjson.loads(fstream.read())
                deps = depDocument["attributes"]
                # Add all attributes to this document
                self.document["attributes"].update(deps)
                fstream.close()

        if "components" in self.document:
            IDLDocument.BeginNamespace(f, self.document)
            namespace = IDLDocument.GetNamespace(self.document)
            for componentName, component in self.document["components"].iteritems():
                f.WriteLine("")
                componentWriter = IDLComponent.ComponentClassWriter(f, self.document, component, componentName, namespace)
                componentWriter.WriteClassImplementation()
                f.WriteLine("")
            IDLDocument.EndNamespace(f, self.document)

        f.Close()
