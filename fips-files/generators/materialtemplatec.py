import os, platform, sys
import sjson
import ntpath
from pathlib import Path
Version = 1

import sys
if __name__ == '__main__':
    sys.path.insert(0,'../fips/generators')
    sys.path.insert(0,'../../../fips/generators')

import genutil as util
import IDLC
import IDLC.filewriter

class StructDeclaration:
    def __init__(self, name):
        self.name = name
        self.members = list()
        self.structs = list()

    def AddMember(self, type, name):
        self.members.append(dict(name = name, type = type))

    def AddStruct(self, struct):
        self.structs.append(struct)
        
    def Format(self, indent):
        str = ''
        for struct in self.structs:
            str += struct.Format(indent + '\t')
        for member in self.members:
            str += '{}\t{} {};\n'.format(indent, member["type"], member["name"])
        return '{}struct {}\n{}{{\n{}{}}};\n'.format(indent, self.name, indent, str, indent)
    
class VariableDefinition:
    def __init__(self, name, ty, default):
        self.name = name
        self.type = ty
        self.default = default

    def __hash__(self):
        return hash(self.name)
    
    def __eq__(self, other):
        return self.name == other.name

class PassDefinition:
    def __init__(self, batch, shader, variation):
        self.batch = batch
        self.shader = shader
        self.variation = variation

    def __hash__(self):
        return hash(self.batch)

    def __eq__(self, other):
        return self.batch == other.batch

class MaterialTemplateDefinition:
    def __init__(self, node):
        self.name = node['name']
        self.name = self.name.replace(" ", "_").replace("+", "")
        self.inherits = ""
        self.virtual = False
        self.passes = set()
        self.variables = set()
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
                    self.variables.add(VariableDefinition(varName, varType, varDef))
        if "passes" in node:
            for p in node["passes"]:
                    batch = p["batch"]
                    shader = p["shader"]
                    variation = p["variation"]
                    self.passes.add(PassDefinition(batch, shader, variation))
    pass

    def FormatHeader(self):
        #ret = 'struct {}\n{{\n'.format(self.name)
        ret = ''

        if self.virtual:
            ret += "\t/* Virtual Material */\n"
        ret += '\tconst char* Description = "{}";\n'.format(self.desc)
        if not self.virtual:
            ret += '\tEntry entry = {{.name = "{}", .properties = Materials::MaterialProperties::{}, .vertexLayout =  CoreGraphics::{}}};\n'.format(self.name, self.properties, self.vertex)
            ret += '\tvoid Setup();\n'

        ret = 'struct {}\n{{\n{}}};\n'.format(self.name, ret)
        ret += 'extern struct {} __{};'.format(self.name, self.name)
        return ret
    
    def FormatSource(self):
        if not self.virtual:
            func = ''
            numTextures = 0
            numConstants = 0
            func += '\tthis->entry.constantsPerBatch.Resize({});\n'.format(len(self.passes))
            func += '\tthis->entry.texturesPerBatch.Resize({});\n'.format(len(self.passes))
            func += '\tthis->entry.constantBatchLookup.Resize({});\n'.format(len(self.passes))
            func += '\tthis->entry.textureBatchLookup.Resize({});\n'.format(len(self.passes))
            defList = list()
            for var in self.variables:
                varStr = ''
                typeStr = ''
                accessorStr = ''
                if var.type == "vec4" or var.type == "vec3" or var.type == "vec2":
                    vecStr = ''
                    numConstants += 1
                    for val in var.default:
                        vecStr += '{}, '.format(val)
                    vecStr = vecStr[:-2]
                    if var.type == 'vec4':
                        typeStr = 'Vec4'
                        varStr += 'Math::vec4({})'.format(vecStr)
                        accessorStr = 'f4'
                    elif var.type == 'vec3':
                        typeStr = 'Vec3'
                        varStr += 'Math::vec3({})'.format(vecStr)
                        accessorStr = 'f3'
                    elif var.type == 'vec2':
                        typeStr = 'Vec2'
                        varStr += 'Math::vec2({})'.format(vecStr)
                        accessorStr = 'f2'
                elif var.type == "float":
                    typeStr = 'Scalar'
                    varStr += '{}'.format(var.default)
                    accessorStr = 'f'
                    numConstants += 1
                elif var.type == "bool":
                    typeStr = 'Bool'
                    varStr += '{}'.format(var.default).lower()
                    accessorStr = 'b'
                    numConstants += 1
                elif var.type == "textureHandle":
                    typeStr = 'BindlessResource'
                    varStr += '"{}"'.format(var.default)
                    accessorStr = 'resource'
                    numConstants += 1
                elif var.type == "texture2d":
                    typeStr = 'Resource'
                    varStr += '"{}"'.format(var.default)
                    accessorStr = 'resource'
                    numTextures += 1

                func += '\tthis->entry.values.Add("{}", Materials::MaterialTemplateValue{{.type = Materials::MaterialTemplateValue::Type::{}, .data = {{.{} = {}}} }});\n'.format(var.name, typeStr, accessorStr, varStr)
                defList.append('Materials::MaterialTemplateValue{{.type = Materials::MaterialTemplateValue::Type::{}, .data = {{.{} = {}}} }}'.format(typeStr, accessorStr, varStr))
            passCounter = 0
            for p in self.passes:
                func += '\t{\n'
                func += '\t\t/* Pass {} */\n'.format(p.batch)
                func += '\t\tCoreGraphics::ShaderId shader = CoreGraphics::ShaderServer::Instance()->GetShader("shd:{}.fxb");\n'.format(p.shader)
                func += '\t\tCoreGraphics::ShaderProgramId program = CoreGraphics::ShaderGetProgram({}, CoreGraphics::ShaderFeatureFromString("{}"));\n'.format('shader', p.variation)
                func += '\t\tthis->entry.passes.Add(CoreGraphics::BatchGroup::FromName("{}"), Entry::Pass{{.shader = shader, .program = program, .index = {} }});\n'.format(p.batch, passCounter)
                func += '\t\tthis->entry.texturesPerBatch[{}].Resize({});\n'.format(passCounter, numTextures)
                func += '\t\tthis->entry.constantsPerBatch[{}].Resize({});\n'.format(passCounter, numConstants)
                texCounter = 0
                constCounter = 0
                varCounter = 0
                for var in self.variables:
                    if var.type == 'texture2d':
                        func += '\t\tthis->entry.textureBatchLookup[{}].Add("{}", this->entry.texturesPerBatch[{}].Size());\n'.format(passCounter, var.name, texCounter)
                        func += '\t\tthis->entry.texturesPerBatch[{}][{}] = Materials::ShaderConfigBatchTexture{{.slot = CoreGraphics::ShaderGetResourceSlot(shader, "{}"), .def = {}}};\n'.format(passCounter, texCounter, var.name, defList[varCounter])
                        texCounter += 1
                    else:
                        constStr = '\t\t\t{}Constant.slot = {}Slot;\n'.format(var.name, var.name)
                        constStr += '\t\t\t{}Constant.offset = CoreGraphics::ShaderGetConstantBinding(shader, "{}");\n'.format(var.name, var.name)
                        constStr += '\t\t\t{}Constant.group = CoreGraphics::ShaderGetConstantGroup(shader, "{}");\n'.format(var.name, var.name)
                        constStr += '\t\t\t{}Constant.def = {};\n'.format(var.name, defList[varCounter])
                        func += '\n\t\tMaterials::ShaderConfigBatchConstant {}Constant;\n'.format(var.name)
                        func += '\t\tIndexT {}Slot = CoreGraphics::ShaderGetConstantSlot(shader, "{}");\n'.format(var.name, var.name)
                        func += '\t\tif ({}Slot != InvalidIndex)\n\t\t{{\n{}\t\t}}\n\t\telse\n\t\t{{\n\t\t\t{}Constant = {{InvalidIndex, InvalidIndex, InvalidIndex}};  \n\t\t}}\n'.format(var.name, constStr, var.name)
                        func += '\t\tthis->entry.constantBatchLookup[{}].Add("{}", this->entry.constantsPerBatch[{}].Size());\n'.format(passCounter, var.name, constCounter)
                        func += '\t\tthis->entry.constantsPerBatch[{}][{}] = {}Constant;\n'.format(passCounter, constCounter, var.name)
                        constCounter += 1
                    varCounter += 1
                func += '\t}\n'
                passCounter += 1
            return '\n//------------------------------------------------------------------------------\n/**\n*/\nvoid\n{}::Setup() \n{{\n{}}}'.format(self.name, func)


class MaterialTemplateGenerator:
    def __init__(self):
        self.document = None
        self.documentPath = ""
        self.version = 0
        self.materials = list()
        self.materialDict = {}
        self.name = ""

    #------------------------------------------------------------------------------
    ##
    #
    def SetVersion(self, v):
        self.version = v

    #------------------------------------------------------------------------------
    ##
    #
    def SetName(self, n):
        self.name = n

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
    def Parse(self):
        if "Nebula" in self.document:
            main = self.document["Nebula"]
            for name, node in main.items():
                    if name == "Templates":
                        for mat in node:
                            matDef = MaterialTemplateDefinition(mat)
                            if matDef.inherits:
                                inheritances = matDef.inherits.split("|")
                                for inherits in inheritances:
                                    adjustedInherits = inherits.replace(" ", "_").replace("+", "")
                                    matDef.variables = self.materialDict[adjustedInherits].variables.union(matDef.variables)
                                    matDef.passes = self.materialDict[adjustedInherits].passes.union(matDef.passes)
                            self.materialDict[matDef.name] = matDef
                            self.materials.append(matDef)

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
        f.WriteLine('#include "util/dictionary.h"')
        f.WriteLine('#include "util/tupleutility.h"')
        f.WriteLine('#include "coregraphics/vertexlayout.h"')
        f.WriteLine('#include "materials/shaderconfig.h"')
        f.WriteLine('#include "math/vec2.h"')
        f.WriteLine('#include "math/vec3.h"')
        f.WriteLine('#include "math/vec4.h"')
        f.WriteLine('using namespace Util;')
        f.WriteLine('namespace MaterialTemplates\n{\n')

        f.WriteLine('namespace {}\n{{\n'.format(self.name))

        passDeclaration = StructDeclaration('Pass')
        passDeclaration.AddMember('CoreGraphics::ShaderId', 'shader')
        passDeclaration.AddMember('CoreGraphics::ShaderProgramId', 'program')
        passDeclaration.AddMember('uint', 'index')

        #entryDeclaration = StructDeclaration('Entry')
        #entryDeclaration.AddStruct(passDeclaration)
        #entryDeclaration.AddMember('const char*', 'name')
        #entryDeclaration.AddMember('Materials::MaterialProperties', 'properties')
        #entryDeclaration.AddMember('CoreGraphics::VertexLayoutType', 'vertexLayout')
        #entryDeclaration.AddMember('Util::Dictionary<const char*, Materials::MaterialTemplateValue>', 'values')
        #entryDeclaration.AddMember('Util::Dictionary<CoreGraphics::BatchGroup::Code, Pass>', 'passes')
        #entryDeclaration.AddMember('Util::Array<Util::Array<Materials::ShaderConfigBatchTexture>>', 'texturesPerBatch')
        #entryDeclaration.AddMember('Util::Array<Util::Array<Materials::ShaderConfigBatchConstant>>', 'constantsPerBatch')
        #entryDeclaration.AddMember('Util::Array<Util::Dictionary<const char*, uint>>', 'textureBatchLookup')
        #entryDeclaration.AddMember('Util::Array<Util::Dictionary<const char*, uint>>', 'constantBatchLookup')
        #f.WriteLine(entryDeclaration.Format(''))

        enumStr = ''
        for mat in self.materials:
            if not mat.virtual:
                enumStr += '\t{},\n'.format(mat.name)

        f.WriteLine('enum MaterialTemplateEnums \n{{\n{}}};\n'.format(enumStr))

        f.WriteLine('extern Util::Dictionary<uint, Entry> Lookup;')
        f.WriteLine('void SetupMaterialTemplates();\n')

        for mat in self.materials:
            f.WriteLine(mat.FormatHeader())

        f.WriteLine('}} // namespace {}\n'.format(self.name))

        f.WriteLine('} // namespace MaterialTemplates\n')
        f.Close()
        return self.materials

    #------------------------------------------------------------------------------
    ##
    #
    def GenerateSource(self, headerPath, outPath):
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
        f.WriteLine('#include "{}"'.format(headerPath))
        f.WriteLine('#include "util/string.h"')
        f.WriteLine('#include "util/dictionary.h"')
        f.WriteLine('#include "coregraphics/vertexlayout.h"')
        f.WriteLine('#include "materials/shaderconfig.h"')
        f.WriteLine('#include "math/vec2.h"')
        f.WriteLine('#include "math/vec3.h"')
        f.WriteLine('#include "math/vec4.h"')
        f.WriteLine('using namespace Util;')
        f.WriteLine('namespace MaterialTemplates\n{\n')

        f.WriteLine('namespace {}\n{{\n'.format(self.name))

        f.WriteLine('// Entry points')
        f.WriteLine('Util::Dictionary<uint, Entry> Lookup;')

        setupStr = ''
        for mat in self.materials:
            if not mat.virtual:
                f.WriteLine(mat.FormatSource())
                setupStr += '\t__{}.Setup();\n'.format(mat.name)
                setupStr += '\tLookup.Add("{}"_hash, __{}.entry);\n\n'.format(mat.name, mat.name)
                f.WriteLine('struct MaterialTemplates::{}::{} MaterialTemplates::{}::__{};'.format(self.name, mat.name, self.name, mat.name))
        f.WriteLine('//------------------------------------------------------------------------------\n/**\n*/\nvoid\nSetupMaterialTemplates()\n{{\n{}}}\n'.format(setupStr))

        f.WriteLine('}} // namespace {}\n'.format(self.name))

        f.WriteLine('} // namespace MaterialTemplates\n')

    #------------------------------------------------------------------------------
    ##
    #
    def GenerateGlueHeader(self, outPath):
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
        f.WriteLine('#include "materials/shaderconfig.h"')

        f.WriteLine('namespace MaterialTemplates\n{\n')

        f.WriteLine('extern Util::Dictionary<uint, Entry> Lookup;\n')
        f.WriteLine('void SetupMaterialTemplates();\n')

        f.WriteLine('} // namespace MaterialTemplates\n')

    #------------------------------------------------------------------------------
    ##
    #
    def GenerateGlueSource(self, files, headerPath, outPath):
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
        f.WriteLine('#include "{}"'.format(headerPath))

        setupStr = ''
        for file in files:
            name = Path(file).stem
            setupStr += '\t{}::SetupMaterialTemplates();\n'.format(name)
            setupStr += '\tLookup.Merge({}::Lookup);\n'.format(name)
            f.WriteLine('#include "{}"'.format(file))

        f.WriteLine('namespace MaterialTemplates\n{\n')
        f.WriteLine('Util::Dictionary<uint, Entry> Lookup;\n')

        f.WriteLine('//------------------------------------------------------------------------------\n/**\n*/\nvoid\nSetupMaterialTemplates() \n{{\n{}}}'.format(setupStr))

        f.WriteLine('} // namespace MaterialTemplates\n')


# Entry point for generator
if __name__ == '__main__':
    globals()
    generator = MaterialTemplateGenerator()
    generator.SetVersion(Version)

    if sys.argv[1] == '--glue':
        generator.GenerateGlueHeader(sys.argv[-2])
        generator.GenerateGlueSource(sys.argv[2:-2], sys.argv[-2], sys.argv[-1])

    else:

        # The number of input files is defined between 1 - len-2
        path = Path(sys.argv[1]).stem
        generator.SetDocument(sys.argv[1])
        generator.Parse()
        generator.SetName(path)
        generator.GenerateHeader(sys.argv[-2])
        generator.GenerateSource(sys.argv[-2], sys.argv[-1])