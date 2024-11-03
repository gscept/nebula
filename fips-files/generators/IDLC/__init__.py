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

        if "messages" in self.document:
            IDLDocument.AddInclude(f, "game/messaging/message.h")
            
        IDLComponent.ParseComponents(self.document)

        IDLDocument.AddInclude(f, "core/types.h")
        if (IDLComponent.ContainsResourceTypes()):
            IDLDocument.AddInclude(f, "resources/resource.h")
        if (IDLComponent.ContainsEntityTypes()):
            IDLDocument.AddInclude(f, "game/entity.h")

        IDLDocument.WriteIncludeHeader(f)
        IDLDocument.WriteIncludes(f, self.document)
        IDLDocument.WriteIncludes(f, componentLibraries)
        
        hasMessages = "messages" in self.document
        hasComponents = "components" in self.document
        hasEnums = "enums" in self.document
        if hasComponents or hasMessages or hasEnums:
            IDLDocument.BeginNamespace(f, self.document)
            
            if hasEnums:
                IDLComponent.WriteEnumeratedCppTypes(f, self.document)

            if hasMessages:
                IDLProtocol.WriteMessageDeclarations(f, self.document)

            if hasComponents:
                IDLComponent.WriteComponentHeaderDeclarations(f, self.document)
                f.WriteLine("")

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
        IDLDocument.AddInclude(f, "memdb/attributeregistry.h")
        IDLDocument.AddInclude(f, "game/componentserialization.h")
        IDLDocument.AddInclude(f, "game/componentinspection.h")
        
        hasMessages = "messages" in self.document

        hasComponents = "components" in self.document
        hasEnums = "enums" in self.document

        if hasEnums or hasComponents:
            IDLDocument.AddInclude(f, "pjson/pjson.h");
            IDLDocument.BeginNamespaceOverride(f, self.document, "IO")
            if hasEnums:
                IDLComponent.WriteEnumJsonSerializers(f, self.document);
            if hasComponents:
                IDLComponent.WriteStructJsonSerializers(f, self.document);
                
            IDLDocument.EndNamespaceOverride(f, self.document, "IO")

        if hasMessages:
            IDLDocument.AddInclude(f, "nanobind/nanobind.h")
            IDLDocument.BeginNamespace(f, self.document)

            if hasMessages:
                IDLProtocol.WriteMessageImplementation(f, self.document)

            IDLDocument.EndNamespace(f, self.document)

        f.Close()

    #------------------------------------------------------------------------------
    ##
    #
    def GenerateCs(self, csPath) :
        f = filewriter.FileWriter()
        f.Open(csPath)        
        f.WriteLine("// NIDL #version:{}#".format(self.version))              
        
        IDLComponent.ParseComponents(self.document)

        IDLDocument.WriteCSHeader(f)        
        f.WriteLine("using System;")
        f.WriteLine("using System.Runtime.CompilerServices;")
        f.WriteLine("using System.Runtime.InteropServices;")
        f.WriteLine("using Nebula.Game;")

        hasComponents = "components" in self.document
        hasEnums = "enums" in self.document

        if hasEnums or hasComponents:
            IDLDocument.BeginNamespace(f, self.document)

            if hasEnums:
                IDLComponent.WriteEnumeratedCsTypes(f, self.document)

            if hasComponents:
                IDLComponent.WriteComponentCsDeclarations(f, self.document)
                f.WriteLine("")

            IDLDocument.EndNamespace(f, self.document)

        f.Close()
