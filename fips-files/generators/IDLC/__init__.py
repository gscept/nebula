import os, platform, sys
import IDLC.idldocument as IDLDocument
import IDLC.idlproperty as IDLProperty
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

        propertyLibraries = []

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fileName = '{}.h'.format(os.path.splitext(dependency)[0]).lower()
                propertyLibraries.append(fileName)

        if "messages" in self.document:
            propertyLibraries.append("game/messaging/message.h")

        propertyLibraries.append("core/sysfunc.h")
        propertyLibraries.append("util/stringatom.h")
        propertyLibraries.append("memdb/typeregistry.h")

        IDLDocument.WriteIncludeHeader(f)
        IDLDocument.WriteIncludes(f, self.document)
        IDLDocument.WriteIncludes(f, propertyLibraries)


        hasMessages = "messages" in self.document
        hasProperties = "properties" in self.document
        hasEnums = "enums" in self.document
        if hasProperties or hasMessages or hasEnums:
            IDLDocument.BeginNamespace(f, self.document)
            
            if hasEnums:
                IDLProperty.WriteEnumeratedTypes(f, self.document)

            if hasMessages:
                IDLProtocol.WriteMessageDeclarations(f, self.document)

            if hasProperties:
                IDLProperty.WritePropertyHeaderDeclarations(f, self.document)
                IDLDocument.BeginNamespaceOverride(f, self.document, "Details")
                IDLProperty.WritePropertyHeaderDetails(f, self.document)
                IDLDocument.EndNamespaceOverride(f, self.document, "Details")
                f.WriteLine("")

            # Add additional dependencies to document.
            if "dependencies" in self.document:
                for dependency in self.document["dependencies"]:
                    fstream = open(dependency, 'r')
                    depDocument = sjson.loads(fstream.read())
                    deps = depDocument["properties"]
                    # Add all properties to this document
                    self.document["properties"].update(deps)
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
        IDLDocument.AddInclude(f, hdrInclude)
        
        hasMessages = "messages" in self.document

        if hasMessages:            
            IDLDocument.AddInclude(f, "scripting/bindings.h")

        # Add additional dependencies to document.
        if "dependencies" in self.document:
            for dependency in self.document["dependencies"]:
                fstream = open(dependency, 'r')
                depDocument = sjson.loads(fstream.read())
                deps = depDocument["properties"]
                # Add all properties to this document
                self.document["properties"].update(deps)
                fstream.close()

        hasProperties = "properties" in self.document
        if hasProperties or hasMessages:
            IDLDocument.BeginNamespace(f, self.document)

            if hasMessages:
                IDLProtocol.WriteMessageImplementation(f, self.document)

            if "properties" in self.document:
                IDLDocument.BeginNamespaceOverride(f, self.document, "Details")
                IDLProperty.WritePropertySourceDefinitions(f, self.document)
                IDLDocument.EndNamespaceOverride(f, self.document, "Details")
                f.WriteLine("")

            IDLDocument.EndNamespace(f, self.document)

        f.Close()
