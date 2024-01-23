import os, platform, sys
import sjson
import ntpath
Version = 1

import sys
if __name__ == '__main__':
    sys.path.insert(0,'../fips/generators')
    sys.path.insert(0,'../../../fips/generators')

import genutil as util
import IDLC
import IDLC.filewriter

class VariableDefinition:
    def __init__(self, name, ty, default):
        self.name = name
        self.type = ty
        self.default = default

class MaterialTemplateDefinition:
    def __init__(self, node):
        self.name = node['name']
        self.name = self.name.replace(" ", "_").replace("+", "")
        self.inherits = ""
        self.virtual = False
        self.passes = list()
        self.variables = list()
        self.properties = "Unknown"
        self.vertex = "Unknown"
        self.group = "Unknown"
        if "inherits" in node:
            self.inherits = node["inherits"]
        if "virtual" in node:
            self.virtual = node["virtual"]

        if not self.virtual:
            self.properties = node["materialProperties"]
            self.vertex = node["vertexType"]
        if "group" in node:
            self.group = node["group"]
        self.desc = node["desc"]
        if "variables" in node:
            for var in node["variables"]:
                    varName = var["name"]
                    varType = var["type"]
                    varDef = var["defaultValue"]
                    self.variables.append(VariableDefinition(varName, varType, varDef))
        if "passes" in node:
            for p in node["passes"]:
                    batch = p["batch"]
                    shader = p["shader"]
                    variation = p["variation"]
                    self.passes.append({"batch": batch, "shader": shader, "variation": variation})
    pass

    def Format(self):
        #ret = 'struct {}\n{{\n'.format(self.name)
        ret = ''
        if self.virtual:
            ret += "\t/* Virtual Material */\n"
        ret += '\tstatic constexpr const char* Description = "{}";\n'.format(self.desc)
        if not self.virtual:
            ret += '\tstatic constexpr Materials::MaterialProperties Properties = Materials::MaterialProperties::{};\n'.format(self.properties)
            ret += '\tstatic constexpr CoreGraphics::VertexLayoutType VertexLayout = CoreGraphics::{};\n'.format(self.vertex)

            for var in self.variables:
                varStr = ''
                if var.type == "vec4" or var.type == "vec3" or var.type == "vec2":
                    vecStr = ''
                    for val in var.default:
                        vecStr += '{}, '.format(val)
                    vecStr = vecStr[:-2]
                    varStr += '\t\tstatic constexpr float Default[{}] = {{{}}};\n'.format(len(var.default), vecStr)
                elif var.type == "float":
                    varStr += '\t\tstatic constexpr {} Default = {}f;\n'.format(var.type, var.default)
                elif var.type == "textureHandle" or var.type == "texture2d":
                    varStr += '\t\tstatic constexpr const char* Default = "{}";\n'.format(var.default)
                
                ret += '\tstruct {}\n\t{{\n{}\t}};\n\n'.format(var.name, varStr)

        ret = 'struct {}\n{{\n{}}};\n'.format(self.name, ret)
        return ret
    pass

class MaterialTemplateGenerator:
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
    def SetDocument(self, input):
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
    def GenerateHeader(self, outPath):
        f = IDLC.filewriter.FileWriter()
        f.Open(outPath)
        f.WriteLine("// Material Template #version:{}#".format(self.version))
        f.WriteLine("#pragma once")
        f.WriteLine("//------------------------------------------------------------------------------")
        f.WriteLine("/**")
        f.IncreaseIndent()
        f.WriteLine("This file was generated with Nebula's Material Template compiler tool.")
        f.WriteLine("DO NOT EDIT")
        f.DecreaseIndent()
        f.WriteLine("*/")
        f.WriteLine('#include "util/string.h"')
        f.WriteLine('#include "coregraphics/vertexlayout.h"')
        f.WriteLine('#include "materials/shaderconfig.h"')
        f.WriteLine('using namespace Util;')
        f.WriteLine('namespace Materials\n{\n')

        self.materials = list()
        self.materialDict = {}

        if "Nebula" in self.document:
            for name, node in self.document["Nebula"].items():
                    if name == "Materials":
                        for mat in node:
                            matDef = MaterialTemplateDefinition(mat)
                            if matDef.inherits:
                                inheritances = matDef.inherits.split("|")
                                for inherits in inheritances:
                                    adjustedInherits = inherits.replace(" ", "_").replace("+", "")
                                    matDef.variables = self.materialDict[adjustedInherits].variables + matDef.variables
                                    matDef.passes = self.materialDict[adjustedInherits].passes + matDef.passes
                            self.materialDict[matDef.name] = matDef
                            self.materials.append(matDef)

        enumStr = ''
        for mat in self.materials:
            if not mat.virtual:
                enumStr += '\t{},\n'.format(mat.name)

        #f.WriteLine('enum MaterialTemplates \n{{\n{}}};\n'.format(enumStr))
        for mat in self.materials:
            f.WriteLine(mat.Format())

        f.WriteLine('} // namespace Materials\n')
        f.Close()



# Entry point for generator
if __name__ == '__main__':
    globals()
    generator = MaterialTemplateGenerator()
    generator.SetVersion(Version)
    generator.SetDocument(sys.argv[1])
    generator.GenerateHeader(sys.argv[2])