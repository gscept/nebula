import os, platform, sys
import IDLC.idldocument as IDLDocument
import IDLC.idlcomponent as IDLComponent
import IDLC.idlprotocol as IDLProtocol
import sjson
import IDLC.filewriter
import genutil as util
import ntpath

class IDLCodeGenerator:
    def __init__(self):
        self.document = None
        self.documentPath = ""
        self.version = 0

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

        componentLibraries = []

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fileName = '{}.h'.format(os.path.splitext(dependency)[0]).lower()
                componentLibraries.append(fileName)

        if "messages" in self.document:
            IDLDocument.AddInclude(f, "game/messaging/message.h")
            
        IDLComponent.ParseComponents(self.document)

        IDLDocument.AddInclude(f, "game/category.h")
        if (IDLComponent.ContainsResourceTypes()):
            IDLDocument.AddInclude(f, "resources/resource.h")
        if (IDLComponent.ContainsEntityTypes()):
            IDLDocument.AddInclude(f, "game/entity.h")

        IDLDocument.WriteIncludeHeader(f)
        IDLDocument.WriteIncludes(f, self.document)
        IDLDocument.WriteIncludes(f, componentLibraries)
        
        IDLComponent.WriteComponentForwardDeclarations(f, self.document)
        
        hasMessages = "messages" in self.document
        hasComponents = "components" in self.document
        hasEnums = "enums" in self.document
        if hasComponents or hasMessages or hasEnums:
            IDLDocument.BeginNamespace(f, self.document)
            
            if hasEnums:
                IDLComponent.WriteEnumeratedTypes(f, self.document)

            if hasMessages:
                IDLProtocol.WriteMessageDeclarations(f, self.document)

            if hasComponents:
                IDLComponent.WriteComponentHeaderDeclarations(f, self.document)
                IDLDocument.BeginNamespaceOverride(f, self.document, "Details")
                IDLComponent.WriteComponentHeaderDetails(f, self.document)
                IDLDocument.EndNamespaceOverride(f, self.document, "Details")
                f.WriteLine("")

            # Add additional dependencies to document.
            if "dependencies" in self.document:
                for dependency in self.document["dependencies"]:
                    fstream = open(dependency, 'r')
                    depDocument = sjson.loads(fstream.read())
                    deps = depDocument["components"]
                    # Add all components to this document
                    self.document["components"].update(deps)
                    fstream.close()

            IDLDocument.EndNamespace(f, self.document)

        f.Close()
        return


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
        IDLDocument.AddInclude(f, "application/stdneb.h")
        IDLDocument.AddInclude(f, hdrInclude)
        IDLDocument.AddInclude(f, "core/sysfunc.h")
        IDLDocument.AddInclude(f, "util/stringatom.h")
        IDLDocument.AddInclude(f, "memdb/typeregistry.h")
        IDLDocument.AddInclude(f, "game/componentserialization.h")
        IDLDocument.AddInclude(f, "game/componentinspection.h")
        
        hasMessages = "messages" in self.document

        if hasMessages:            
            IDLDocument.AddInclude(f, "scripting/python/conversion.h")

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fstream = open(dependency, 'r')
                depDocument = sjson.loads(fstream.read())
                deps = depDocument["components"]
                # Add all components to this document
                self.document["components"].update(deps)
                fstream.close()

        hasStructs = IDLComponent.HasStructComponents()
        hasEnums = "enums" in self.document

        if hasEnums or hasStructs:
            IDLDocument.AddInclude(f, "pjson/pjson.h");
            IDLDocument.BeginNamespaceOverride(f, self.document, "IO")
            if hasEnums:
                IDLComponent.WriteEnumJsonSerializers(f, self.document);
            if hasStructs:
                IDLComponent.WriteStructJsonSerializers(f, self.document);
                
            IDLDocument.EndNamespaceOverride(f, self.document, "IO")

        hasComponents = "components" in self.document
        if hasComponents or hasMessages:
            IDLDocument.BeginNamespace(f, self.document)

            if hasMessages:
                IDLProtocol.WriteMessageImplementation(f, self.document)

            if "components" in self.document:
                IDLComponent.WriteComponentSourceDefinitions(f, self.document)
                f.WriteLine("")

            IDLDocument.EndNamespace(f, self.document)

        f.Close()
