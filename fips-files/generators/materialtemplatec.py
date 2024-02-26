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
        self.uniqueId = 0
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
            for v in self.variables:
                ret += '\tMaterials::MaterialTemplateValue __{};\n'.format(v.name)

            for p in self.passes:
                ret += '\n\tMaterialTemplates::Entry::Pass __{};\n'.format(p.batch)
                for v in self.variables:
                    if v.type == 'texture2d':
                        ret += '\tMaterials::ShaderConfigBatchTexture __{}_{};\n'.format(p.batch, v.name)
                    else:
                        ret += '\tMaterials::ShaderConfigBatchConstant __{}_{};\n'.format(p.batch, v.name)
                        
            ret += '\n\tEntry entry = {{.name = "{}", .uniqueId = {}, .properties = Materials::MaterialProperties::{}, .vertexLayout =  CoreGraphics::{}}};\n'.format(self.name, self.uniqueId, self.properties, self.vertex)
            ret += '\n\tvoid Setup();\n'

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

                func += '\tthis->__{} = Materials::MaterialTemplateValue{{.type = Materials::MaterialTemplateValue::Type::{}, .data = {{.{} = {}}} }};\n'.format(var.name, typeStr, accessorStr, varStr)
                func += '\tthis->entry.values.Add("{}", &this->__{});\n'.format(var.name, var.name)
                defList.append('&this->__{}'.format(var.name))
            passCounter = 0
            for p in self.passes:
                func += '\t{\n'
                func += '\t\t/* Pass {} */\n'.format(p.batch)
                func += '\t\tCoreGraphics::ShaderId shader = CoreGraphics::ShaderGet("shd:{}.fxb");\n'.format(p.shader)
                func += '\t\tCoreGraphics::ShaderProgramId program = CoreGraphics::ShaderGetProgram({}, CoreGraphics::ShaderFeatureMask("{}"));\n'.format('shader', p.variation)
                func += '\t\tthis->__{} = Entry::Pass{{.shader = shader, .program = program, .index = {} }};\n'.format(p.batch, passCounter)
                func += '\t\tthis->entry.passes.Add(CoreGraphics::BatchGroup::FromName("{}"), &this->__{});\n'.format(p.batch, p.batch)
                func += '\t\tthis->entry.texturesPerBatch[{}].Resize({});\n'.format(passCounter, numTextures)
                func += '\t\tthis->entry.constantsPerBatch[{}].Resize({});\n'.format(passCounter, numConstants)
                texCounter = 0
                constCounter = 0
                varCounter = 0
                for var in self.variables:
                    memStr = 'this->__{}_{}'.format(p.batch, var.name)
                    if var.type == 'texture2d':
                        func += '\t\tthis->entry.textureBatchLookup[{}].Add("{}"_hash, {});\n'.format(passCounter, var.name, texCounter)
                        func += '\t\t{} = Materials::ShaderConfigBatchTexture{{.slot = CoreGraphics::ShaderGetResourceSlot(shader, "{}"), .def = {}}};\n'.format(memStr, var.name, defList[varCounter]);
                        func += '\t\tthis->entry.texturesPerBatch[{}][{}] = &{};\n'.format(passCounter, texCounter, memStr)
                        texCounter += 1
                    else:
                        constStr = '\t\t\t{}.slot = {}Slot;\n'.format(memStr, var.name)
                        constStr += '\t\t\t{}.offset = CoreGraphics::ShaderGetConstantBinding(shader, "{}");\n'.format(memStr, var.name)
                        constStr += '\t\t\t{}.group = CoreGraphics::ShaderGetConstantGroup(shader, "{}");\n'.format(memStr, var.name)
                        constStr += '\t\t\t{}.def = {};\n'.format(memStr, defList[varCounter])
                        func += '\t\tIndexT {}Slot = CoreGraphics::ShaderGetConstantSlot(shader, "{}");\n'.format(var.name, var.name)
                        func += '\t\tif ({}Slot != InvalidIndex)\n\t\t{{\n{}\t\t}}\n\t\telse\n\t\t{{\n\t\t\t{} = {{InvalidIndex, InvalidIndex, InvalidIndex}};  \n\t\t}}\n'.format(var.name, constStr, memStr)
                        func += '\t\tthis->entry.constantBatchLookup[{}].Add("{}"_hash, {});\n'.format(passCounter, var.name, constCounter)
                        func += '\t\tthis->entry.constantsPerBatch[{}][{}] = &{};\n'.format(passCounter, constCounter, memStr)
                        constCounter += 1
                    varCounter += 1
                func += '\t}\n'
                passCounter += 1
            return '\n//------------------------------------------------------------------------------\n/**\n*/\nvoid\n{}::Setup() \n{{\n{}}}'.format(self.name, func)


materialCounter = 0
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
        global materialCounter;

        if "Nebula" in self.document:
            main = self.document["Nebula"]
            for name, node in main.items():
                    if name == "Templates":
                        for mat in node:
                            matDef = MaterialTemplateDefinition(mat)
                            matDef.uniqueId = materialCounter;
                            materialCounter += 1;
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
        f.WriteLine('#include "coregraphics/shader.h"')

        f.WriteLine('#include "math/vec2.h"')
        f.WriteLine('#include "math/vec3.h"')
        f.WriteLine('#include "math/vec4.h"')
        f.WriteLine('using namespace Util;')
        f.WriteLine('namespace MaterialTemplates\n{\n')

        f.WriteLine('namespace {}\n{{\n'.format(self.name))

        enumStr = ''
        for mat in self.materials:
            if not mat.virtual:
                enumStr += '\t{},\n'.format(mat.name)

        f.WriteLine('enum MaterialTemplateEnums \n{{\n{}}};\n'.format(enumStr))
        f.WriteLine('void SetupMaterialTemplates(Util::Dictionary<uint, Entry*>& Lookup, Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<Entry*>>& Configs);\n')

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
        setupStr = ''
        for mat in self.materials:
            if not mat.virtual:
                f.WriteLine(mat.FormatSource())
                setupStr += '\t__{}.Setup();\n'.format(mat.name)
                setupStr += '\tLookup.Add("{}"_hash, &__{}.entry);\n'.format(mat.name, mat.name)
                for p in mat.passes:
                    setupStr += '\tConfigs.Emplace(CoreGraphics::BatchGroup::FromName("{}")).Append(&__{}.entry);\n'.format(p.batch, mat.name)
                setupStr += '\n'
                f.WriteLine('struct MaterialTemplates::{}::{} MaterialTemplates::{}::__{};'.format(self.name, mat.name, self.name, mat.name))
        f.WriteLine('//------------------------------------------------------------------------------\n/**\n*/\nvoid\nSetupMaterialTemplates(Util::Dictionary<uint, Entry*>& Lookup, Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<Entry*>>& Configs)\n{{\n{}}}\n'.format(setupStr))

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

        f.WriteLine('extern Util::Dictionary<uint, Entry*> Lookup;')
        f.WriteLine('extern Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<Entry*>> Configs;\n')
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
            setupStr += '\t{}::SetupMaterialTemplates(Lookup, Configs);\n'.format(name)
            f.WriteLine('#include "{}"'.format(file))

        f.WriteLine('namespace MaterialTemplates\n{\n')
        f.WriteLine('Util::Dictionary<uint, Entry*> Lookup;')
        f.WriteLine('Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<Entry*>> Configs;\n')

        f.WriteLine('//------------------------------------------------------------------------------\n/**\n*/\nvoid\nSetupMaterialTemplates() \n{{\n{}}}'.format(setupStr))

        f.WriteLine('} // namespace MaterialTemplates\n')


# Entry point for generator
if __name__ == '__main__':
    globals()
    generator = MaterialTemplateGenerator()
    generator.SetVersion(Version)

    if sys.argv[1] == '--glue':
        print("Compiling glue '{}' -> '{}' & '{}'".format(sys.argv[2:-2], sys.argv[-2], sys.argv[-1]))
        generator.GenerateGlueHeader(sys.argv[-2])
        generator.GenerateGlueSource(sys.argv[2:-2], sys.argv[-2], sys.argv[-1])

    else:

        # The number of input files is defined between 1 - len-2
        path = Path(sys.argv[1]).stem
        print("Compiling material template '{}' -> '{}' & '{}'".format(sys.argv[1], sys.argv[-2], sys.argv[-1]))
        generator.SetDocument(sys.argv[1])
        generator.Parse()
        generator.SetName(path)
        generator.GenerateHeader(sys.argv[-2])
        generator.GenerateSource(sys.argv[-2], sys.argv[-1])